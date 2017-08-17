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

klee::ExecutionState *Inception::RealInterrupt::interrupt_state = NULL;

bool RealInterrupt::interrupted = false;

llvm::Function *RealInterrupt::caller = NULL;

klee::Executor *RealInterrupt::executor = NULL;

bool RealInterrupt::ending = false;

uint32_t RealInterrupt::irq_id_base_addr = NULL;

RealInterrupt::RealInterrupt() {}

// void watch_and_avoid(int id) {}

void RealInterrupt::init(klee::Executor *_executor) {

  srand(time(NULL));

  RealInterrupt::executor = _executor;

  ParserInterruptCB callback = &RealInterrupt::AddInterrupt;

  while (Configurator::next_interrupt(callback) == true)
    ;

  irq_id_base_addr = Configurator::getAsInteger("Stub", "address", 0);

  // configure trace
  if (RealTarget::inception_device == NULL)
    RealTarget::inception_device = jtag_init();

  Watcher watcher = &RealInterrupt::raise;
  // Watcher watcher = &watch_and_avoid;
  trace_init(Inception::RealTarget::inception_device, watcher);
}

void RealInterrupt::AddInterrupt(std::string handler_name, uint32_t id,
                                 uint32_t group_priority,
                                 uint32_t internal_priority) {

  printf("AddInterrupt(%s, %d, %d, %d)\n", handler_name.c_str(), id,
         group_priority, internal_priority);

  interrupts_vector.insert(std::pair<uint32_t, Interrupt *>(
      id, new Interrupt(handler_name, id, group_priority, internal_priority)));
}

void RealInterrupt::raise(int id) {

  try {
    Interrupt *interrupt = interrupts_vector.at(id);

    llvm::errs() << "[RealInterrupt] Raise interrupt id : " << id << "\n";

    // When added the priority_queue will sort the queue
    // according to the user defined priority
    RealInterrupt::pending_interrupts.push(interrupt);
  } catch (const std::out_of_range &oor) {

    llvm::errs() << "[RealInterrupt] Unknown interrupt id : " << id << "\n";
  }
}

RealInterrupt::~RealInterrupt() {}

ExecutionState *RealInterrupt::next_without_priority() {

  ExecutionState *state = NULL;

  unsigned fire = rand() % 1000000;

  if (fire == 666) {

    RealInterrupt::raise(27);
  }

  if (RealInterrupt::ending) {
    // printf("[RealInterrupt] ending ...\n");

    if (RealInterrupt::interrupt_state == NULL)
      throw std::runtime_error(
          "[RealInterrupt] Can not end empty interrupt state ...");

    RealInterrupt::executor->getPTree()->remove(
        RealInterrupt::interrupt_state->ptreeNode);
    delete RealInterrupt::interrupt_state;
    RealInterrupt::ending = false;
    RealInterrupt::interrupt_state = NULL;
    return NULL;
  }

  // Are we in an interrupt ?
  if (RealInterrupt::is_interrupted())
    return RealInterrupt::interrupt_state;

  if (RealInterrupt::enabled)
    if (!RealInterrupt::pending_interrupts.empty())
      return create_interrupt_state();

  return NULL;
}

/*
 * Return the next
 */
ExecutionState *RealInterrupt::next() {

  ExecutionState *state = NULL;

  if (RealInterrupt::ending) {

    if (RealInterrupt::interrupt_state == NULL)
      throw std::runtime_error(
          "[RealInterrupt] Can not end empty interrupt state ...");

    RealInterrupt::executor->getPTree()->remove(
        RealInterrupt::interrupt_state->ptreeNode);
    delete RealInterrupt::interrupt_state;
    RealInterrupt::ending = false;
    RealInterrupt::interrupt_state = NULL;
    return NULL;
  }

  // Are we in an interrupt ?
  if (RealInterrupt::is_interrupted() && !RealInterrupt::masked())
    // Is there any higher priority waiting ?
    if ((state = RealInterrupt::getPending()) != NULL)
      return state;
    else
      return RealInterrupt::interrupt_state;
  else if (RealInterrupt::is_interrupted() && RealInterrupt::masked())
    return RealInterrupt::interrupt_state;

  if (!RealInterrupt::pending_interrupts.empty())
    return create_interrupt_state();
  else
    return NULL;
}

ExecutionState *RealInterrupt::getPending() {

  Interrupt *interrupt = RealInterrupt::pending_interrupts.top();

  llvm::errs() << "Comparing : " << interrupt->state << " and "
               << interrupt_state << "\n";

  if (interrupt->state == RealInterrupt::interrupt_state)
    return NULL;

  return interrupt->state = RealInterrupt::create_interrupt_state();
}

bool RealInterrupt::masked() { return RealInterrupt::enabled; }

void RealInterrupt::stop_interrupt() {

  Interrupt *interrupt = RealInterrupt::current_interrupt;

  jtag_write(RealTarget::inception_device,
             irq_id_base_addr + (interrupt->id * 4), 0, 32);

  RealInterrupt::interrupted = false;

  RealInterrupt::ending = true;

  // llvm::errs() << "[RealInterrupt] Return from interrupt ...\n\n";

  RealInterrupt::current_interrupt = NULL;
}

bool RealInterrupt::is_interrupted() { return RealInterrupt::interrupted; }

ExecutionState *RealInterrupt::create_interrupt_state() {

  ExecutionState *current = RealInterrupt::executor->getExecutionState();

  // Save the function where we were when the interrupt occured
  // Inception::RealInterrupt::caller =
  // current->pc->inst->getParent()->getParent();

  // bool b =
  // Inception::RealInterrupt::caller->getName().find("klee_overshift_check") !=
  // std::string::npos;
  bool a = current->prevPC->inst->getParent()->getParent()->getName().find(
               "klee_overshift_check") != std::string::npos;

  if (a) {

    // printf("[RealInterrupt] Abort interrupt from %s ...\n",
    // current->prevPC->inst->getParent()->getParent()->getName().str().c_str());
    Inception::RealInterrupt::caller = NULL;
    return NULL;
  }

  Inception::RealInterrupt::caller =
      current->prevPC->inst->getParent()->getParent();

  RealInterrupt::interrupted = true;

  RealInterrupt::current_interrupt = RealInterrupt::pending_interrupts.top();
  RealInterrupt::pending_interrupts.pop();

  // Get the handler name of the interrupt
  llvm::StringRef function_name(RealInterrupt::current_interrupt->handlerName);

  // Retrieve the LLVM Function
  Function *f_interrupt =
      RealInterrupt::executor->getKModule()->module->getFunction(function_name);
  if (f_interrupt == NULL)
    klee_error("[RealInterrupt] Fail to resolve interrupt handler name : ",
               function_name.str());
  else
    llvm::errs() << "[RealInterrupt] Suspending "
                 << Inception::RealInterrupt::caller->getName()
                 << " to execute " << function_name << "\n";

  // Copy the current state
  ExecutionState *_interrupt_state = current->branch();
  // Check the copy
  assert(_interrupt_state);

  _interrupt_state->description = function_name;
  RealInterrupt::current_interrupt->state = _interrupt_state;

  // Save the new interrupt state
  Inception::RealInterrupt::interrupt_state = _interrupt_state;
  interrupt_state->addressSpace.cowKey = current->addressSpace.cowKey;
  interrupt_state->description = function_name;
  interrupt_state->interrupted = true;

  std::pair<PTree::Node *, PTree::Node *> res =
      RealInterrupt::executor->getPTree()->split(current->ptreeNode,
                                                 interrupt_state, current);

  interrupt_state->ptreeNode = res.first;
  current->ptreeNode = res.second;

  KFunction *kf =
      RealInterrupt::executor->getKModule()->functionMap[f_interrupt];
  bool prevOpcode = interrupt_state->prevPC->inst->getOpcode();

  KInstIterator restorePC;

  switch (prevOpcode) {
  case Instruction::Call:
  case Instruction::Ret:
  case Instruction::Br: {
    restorePC = interrupt_state->prevPC;
    interrupt_state->pushFrame(interrupt_state->prevPC, kf);

    // llvm::errs() << "[RealInterrupt] When executing : " << *restorePC->inst
    // << "  "
    // << restorePC->inst->getParent()->getParent()->getName() << "\n";
    //
    // std::string srcFile = restorePC->info->file;
    // if (srcFile.length() > 42)
    //   srcFile = srcFile.substr(42);
    //
    // llvm::errs() << "\t(src line: " << restorePC->info->line << " of " <<
    // srcFile << "\n"; std::vector<StackFrame>::iterator stackSeek =
    // current->stack.begin(); std::vector<StackFrame>::iterator stackEnd =
    // current->stack.end(); int stack_idx = 0; errs() << "asm line " <<
    // restorePC->info->assemblyLine << "\n"; while (stackSeek != stackEnd) {
    //   errs() << "stack idx " << stack_idx << " in ";
    //   errs() << stackSeek->kf->function->getName();
    //
    //   if (stackSeek->caller) {
    //     errs() << " line " << stackSeek->caller->info->assemblyLine;
    //     errs() << "\n";
    //   } else {
    //     errs() << " no caller\n";
    //   }
    //   ++stackSeek;
    //   ++stack_idx;
    // }

    break;
  }
  default:
    restorePC = interrupt_state->prevPC;
    ++restorePC;
    interrupt_state->pushFrame(interrupt_state->prevPC, kf);
    break;
  }

  interrupt_state->pc = kf->instructions;
  return interrupt_state;
}

void RealInterrupt::enable() { enabled = true; }

void RealInterrupt::disable() { enabled = false; }

} // namespace Inception
