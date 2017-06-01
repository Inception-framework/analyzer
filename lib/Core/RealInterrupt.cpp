#include "klee/RealInterrupt.h"
#include "klee/RealTarget.h"
#include "klee/Internal/Module/KModule.h"
#include <stdexcept>
#include "Executor.h"
#include "PTree.h"
#include "llvm/Support/ErrorHandling.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/IR/Module.h"

using namespace klee;

namespace Inception{

//INTERRUPT DEFINITIONS
std::map<uint32_t, Interrupt*> RealInterrupt::interrupts_vector;

//INTERRUPTS QUEUE
std::priority_queue<Interrupt*, std::vector<Interrupt*>, InterruptComparator> RealInterrupt::pending_interrupts;


//INTERNAL VARIABLES

Interrupt* RealInterrupt::current_interrupt = NULL;

klee::ExecutionState* Inception::RealInterrupt::interrupt_state = NULL;

bool RealInterrupt::interrupted = false;

llvm::Function* RealInterrupt::caller = NULL;

klee::Executor* RealInterrupt::executor = NULL;

bool RealInterrupt::ending = false;




RealInterrupt::RealInterrupt(){}

void watch_and_avoid(int id) {}

void RealInterrupt::init(klee::Executor* _executor) {

  RealInterrupt::executor = _executor;

  AddInterrupt(StringRef("GPIO0_IRQHandler"), 48, 0, 0);                 /* GINT0_IRQHandler                */
  AddInterrupt(StringRef("GINT0_IRQHandler"), 56, 0, 0);                 /* GINT0_IRQHandler                */
  AddInterrupt(StringRef("GINT1_IRQHandler"), 57, 0, 0);                 /* GINT1_IRQHandler                */
  AddInterrupt(StringRef("SysTick_Handler"), 15, 0, 0);                  /* SysTick Handler              */

  //configure trace
  if(RealTarget::inception_device == NULL)
    RealTarget::inception_device = jtag_init();

  // Watcher watcher = &RealInterrupt::raise;
  Watcher watcher = &watch_and_avoid;
  trace_init(Inception::RealTarget::inception_device, watcher);
}

void RealInterrupt::AddInterrupt(StringRef handler_name, uint32_t id, uint32_t group_priority, uint32_t internal_priority) {

  interrupts_vector.insert(std::pair<uint32_t, Interrupt*>(id, new Interrupt(handler_name, id, group_priority, internal_priority)));
}

void RealInterrupt::raise(int id) {

  try {
    Interrupt* interrupt = interrupts_vector.at(id);

    llvm::errs() << "[RealInterrupt] Raise interrupt id : " << id << "\n";

    //When added the priority_queue will sort the queue
    //according to the user defined priority
    RealInterrupt::pending_interrupts.push(interrupt);
  }
  catch (const std::out_of_range& oor) {

    llvm::errs() << "[RealInterrupt] Unknown interrupt id : " << id << "\n";
  }
}

RealInterrupt::~RealInterrupt(){}

ExecutionState* RealInterrupt::next_without_priority() {

  if(RealInterrupt::pending_interrupts.empty())
    return NULL;
  else
    return RealInterrupt::create_interrupt_state();
}

/*
* Return the next
*/
ExecutionState* RealInterrupt::next() {

  ExecutionState* state = NULL;

  if(RealInterrupt::ending) {

    if(RealInterrupt::interrupt_state == NULL)
      throw std::runtime_error("[RealInterrupt] Can not end empty interrupt state ...");

    RealInterrupt::executor->getPTree()->remove(RealInterrupt::interrupt_state->ptreeNode);
    delete RealInterrupt::interrupt_state;
    RealInterrupt::ending = false;
    RealInterrupt::interrupt_state = NULL;
    return NULL;
  }

  // Are we in an interrupt ?
  if( RealInterrupt::is_interrupted() && !RealInterrupt::masked() )
    // Is there any higher priority waiting ?
    if( (state = RealInterrupt::getPending()) != NULL )
      return state;
    else
      return RealInterrupt::interrupt_state;
  else if( RealInterrupt::is_interrupted() && RealInterrupt::masked() )
    return RealInterrupt::interrupt_state;

  if(!RealInterrupt::pending_interrupts.empty())
    return  create_interrupt_state();
  else
    return NULL;
}

ExecutionState* RealInterrupt::getPending() {

  Interrupt *interrupt = RealInterrupt::pending_interrupts.top();

  llvm::errs() << "Comparing : " << interrupt->state << " and " << interrupt_state << "\n";

  if(interrupt->state == RealInterrupt::interrupt_state)
    return NULL;

  return interrupt->state = RealInterrupt::create_interrupt_state();
}

bool RealInterrupt::masked() {
  return false;
}

void RealInterrupt::stop_interrupt() {

  Interrupt *interrupt = RealInterrupt::current_interrupt;

  jtag_write(RealTarget::inception_device, 0x10004000 + (interrupt->id * 4), 0xAB, 32);

  RealInterrupt::interrupted = false;

  RealInterrupt::ending = true;

  llvm::errs() << "[RealInterrupt] Return from interrupt ...\n\n";

  RealInterrupt::current_interrupt = NULL;
}

bool RealInterrupt::is_interrupted() {

  return RealInterrupt::interrupted;
}

ExecutionState* RealInterrupt::create_interrupt_state() {

  ExecutionState *current = RealInterrupt::executor->getExecutionState();

  RealInterrupt::interrupted = true;

  RealInterrupt::current_interrupt = RealInterrupt::pending_interrupts.top();
  RealInterrupt::pending_interrupts.pop();

  //Save the function where we were when the interrupt occured
  Inception::RealInterrupt::caller = current->pc->inst->getParent()->getParent();

  //Get the handler name of the interrupt
  llvm::StringRef function_name = RealInterrupt::current_interrupt->handlerName;

  //Retrieve the LLVM Function
  Function *f_interrupt = RealInterrupt::executor->getKModule()->module->getFunction(function_name);
  if(f_interrupt == NULL)
    klee_error("[RealInterrupt] Fail to resolve interrupt handler name : ", function_name.str().c_str());
  else
    llvm::errs() << "[RealInterrupt] Raise " << function_name << "\n";

  //Copy the current state
  ExecutionState *_interrupt_state = current->branch();
  //Check the copy
  assert(_interrupt_state);

  _interrupt_state->description = function_name;
  RealInterrupt::current_interrupt->state = _interrupt_state;

  //Save the new interrupt state
  Inception::RealInterrupt::interrupt_state = _interrupt_state;
  interrupt_state->addressSpace.cowKey = current->addressSpace.cowKey;
  interrupt_state->description = function_name;
  interrupt_state->interrupted = true;

  std::pair<PTree::Node *, PTree::Node *> res =
  RealInterrupt::executor->getPTree()->split(current->ptreeNode, interrupt_state, current);

  interrupt_state->ptreeNode = res.first;
  current->ptreeNode = res.second;

  KFunction *kf =  RealInterrupt::executor->getKModule()->functionMap[f_interrupt];
  bool prevOpcode = interrupt_state->prevPC->inst->getOpcode();

  KInstIterator restorePC;

  switch (prevOpcode) {
  case Instruction::Call:
  case Instruction::Ret:
  case Instruction::Br:
    restorePC = current->prevPC;
    interrupt_state->pushFrame(restorePC, kf);
    break;
  default:
    errs() << "will not restore to special pc\n";
    restorePC = current->prevPC;
    ++restorePC;
    interrupt_state->pushFrame(restorePC, kf);
    break;
  }

  interrupt_state->pc = kf->instructions;
  return interrupt_state;
}

}
