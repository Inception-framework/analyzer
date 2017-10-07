#include "inception/RealInterrupt.h"
#include "../Core/Executor.h"
#include "../Core/PTree.h"
#include "inception/Configurator.h"
#include "inception/RealTarget.h"
#include "klee/Internal/Module/InstructionInfoTable.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ErrorHandling.h"
#include <stdexcept>
#include <time.h>

using namespace klee;

namespace Inception {

// INTERRUPT DEFINITIONS
std::map<uint32_t, Interrupt *> RealInterrupt::interrupts_vector;

// INTERRUPTS QUEUE
std::priority_queue<Interrupt *, std::vector<Interrupt *>, InterruptComparator>
    RealInterrupt::pending_interrupts;

bool RealInterrupt::enabled = true;

// INTERNAL VARIABLES

Interrupt *RealInterrupt::current_interrupt = NULL;

bool RealInterrupt::interrupted = false;

llvm::Function *RealInterrupt::caller = NULL;

klee::Executor *RealInterrupt::executor = NULL;

uint32_t RealInterrupt::irq_id_base_addr = 0;

bool RealInterrupt::isDeviceConnected = true;

/*
 * Constructor
 */
RealInterrupt::RealInterrupt() {}

/*
 * Destructor
 */
RealInterrupt::~RealInterrupt() {}

/*
 * This function initializes the interrupt module.
 */
void RealInterrupt::init(klee::Executor *_executor) {

  // Klee executor
  RealInterrupt::executor = _executor;

  // configuration via json
  ParserInterruptCB callback = &RealInterrupt::AddInterrupt;

  while (Configurator::next_interrupt(callback) == true)
    ;

  irq_id_base_addr = Configurator::getAsInteger("Stub", "address", 0);

  if (Configurator::getAsInteger("Analyzer", "Redirection", 0) == 1)
    isDeviceConnected = true;
  else
    isDeviceConnected = false;

  if (isDeviceConnected) {
    // configure trace
    if (RealTarget::inception_device == NULL)
      RealTarget::inception_device = jtag_init();

    Watcher watcher = &RealInterrupt::raise;
    trace_init(Inception::RealTarget::inception_device, watcher);
  }
}

/*
 * Function used to add interrupt id / handler name static mapping
 */
void RealInterrupt::AddInterrupt(std::string handler_name, uint32_t id,
                                 uint32_t group_priority,
                                 uint32_t internal_priority) {

  // printf("AddInterrupt(%s, %d, %d, %d)\n", handler_name.c_str(), id,
  //        group_priority, internal_priority);

  interrupts_vector.insert(std::pair<uint32_t, Interrupt *>(
      id, new Interrupt(handler_name, id, group_priority, internal_priority)));
}

/*
 * Function called by the trace unit to notify the arrival of an interrupt
 */
void RealInterrupt::raise(int id) {

  try {
    Interrupt *interrupt = interrupts_vector.at(id);

    klee_message("Raising interrupt %d", id);

    // When added the priority_queue will sort the queue
    // according to the user defined priority
    RealInterrupt::pending_interrupts.push(interrupt);
  } catch (const std::out_of_range &oor) {
    klee_warning("Unknown interrupt %d ", id);
  }
}

/*
 * This function must be executed when an interrupt handler executes its return
 * instruction (see Executor.cpp).
 * It will exit from the interrupted state, thus allowing other interrupts to be
 * served.
 * It will ack the stub on the device, to terminate the corresponding handler
 * there.
 */
void RealInterrupt::stop_interrupt() {

  // get the interrupt currently being served, set by serve_interrupt().
  Interrupt *interrupt = RealInterrupt::current_interrupt;

  // in case we are in real device mode, ack the stub to terminate the
  // corresponding handler there
  if (isDeviceConnected)
    jtag_write(RealTarget::inception_device,
               irq_id_base_addr + (interrupt->id * 4), 0, 32);

  // exit from the interrupted state and clean the current interrupt
  RealInterrupt::interrupted = false;
  RealInterrupt::current_interrupt = NULL;
}

/*
 * If necessary, this function serves a pending interrupt by calling the
 * corresponding handler. It must be called during the execution loop, after
 * selecting the execution state (see Executor.cpp)
 *
 *
 * IMPORTANT NOTE
 * When an interrupt occurs, we want to inject an instruction in the execution
 * loop (a call to the interrupt handler)
 * Unfortunately, in klee we cannot inject an instruction, at best we can take
 * the instruction pc and transform it into the first instruction of the
 * handler. This corresponds to pretending that prevPC was a call to the
 * handler. Then we have to make sure to come back to the right instruction when
 * we return from the interrupt. The right instruction is of course pc. So the
 * best is to use a trick. In the frame we push caller = pc. In the return, if
 * we are a handler, we set pc = caller. This works whatever insrtuction prevPC
 * is when we "inject" the call to the handler.
 * Instead, if we push the frame with caller = prevPC (normal for a call), the
 * return sets pc = prevPC++. But we have modified pc to become the first
 * instruction of the handler... So basically this cannot work if prevPC is a
 * control flow instruciton that modifies pc...
 *
 *
 * CURRENT LIMITATIONS
 * 1. Interrupt nesting is not allowed
 * 2. Hanlder resolution is static, and here we directly call the handler.
 *    In the future we will call a function resolve(id) which will look in the
 *    vector table in the heap to find the address of the handler,
 *    save the context, call the proper handler with
 *    icp technique, restore the context.
 */
void RealInterrupt::serve_pending_interrupt() {
  // Return immediately if we do not have to serve interrupts (if any of the
  // following conditions is true. The order is chose for efficiency
  if (!RealInterrupt::enabled) // interrupts are disabled
    return;
  if (RealInterrupt::pending_interrupts.empty()) // no interrupt is pending
    return;
  if (RealInterrupt::interrupted) // already serving an interrupt
    return;

  // get the current execution state
  ExecutionState *current = RealInterrupt::executor->getExecutionState();

  // get the caller i.e. who is being interrupted
  Inception::RealInterrupt::caller =
      current->pc->inst->getParent()->getParent();

  // return if the caller is this klee function
  if (caller->getName().find("klee_overshift_check") != std::string::npos)
    return;

  // get the pending interrupt
  RealInterrupt::current_interrupt = RealInterrupt::pending_interrupts.top();
  RealInterrupt::pending_interrupts.pop();

  // get the handler name of the interrupt
  llvm::StringRef function_name(RealInterrupt::current_interrupt->handlerName);

  // get the corresponding LLVM Function
  Function *f_interrupt =
      RealInterrupt::executor->getKModule()->module->getFunction(function_name);
  if (f_interrupt == NULL)
    klee_error("[RealInterrupt] Fail to resolve interrupt handler name : ",
               function_name.str().c_str());
  else
    klee_message("[RealInterrupt] Suspending %s to execute %s ",
                 caller->getName().str().c_str(), function_name.str().c_str());

  // get the klee function
  KFunction *kf =
      RealInterrupt::executor->getKModule()->functionMap[f_interrupt];

  // push a stack frame, saying that the caller is current->pc, see IMPORTANT
  // NOTE for the reason
  current->pushFrame(current->pc, kf);

  // finally "call" the handler by setting the pc to point to it
  current->pc = kf->instructions;

  // flag to mark the interrupted state
  // it is useful in the current version to forbid nested interrupts
  RealInterrupt::interrupted = true;
}

void RealInterrupt::enable() { enabled = true; }

void RealInterrupt::disable() { enabled = false; }

} // namespace Inception
