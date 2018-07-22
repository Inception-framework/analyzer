/*
    This file is part of Inception analyzer.

    Inception-analyzer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Inception analyzer.  If not, see <https://www.gnu.org/licenses/>.

    Copyright (c) 2017 Maxim Integrated, Inc.
    Author: Nassim Corteggiani <nassim.corteggiani@maximintegrated.com>

    Copyright (c) 2017 EURECOM, Inc.
    Author: Giovanni Camurati <giovanni.camurati@eurecom.fr>
*/

#include "inception/RealInterrupt.h"
#include "../Core/Executor.h"
#include "../Core/PTree.h"
#include "../include/klee/Expr.h"
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

uint32_t RealInterrupt::interrupt_vector_base_addr = 0;

uint32_t RealInterrupt::irq_id_base_addr = 0;

bool RealInterrupt::isDeviceConnected = true;

bool RealInterrupt::isDynamicInterruptTable = true;

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

  if (Configurator::getAsInteger("Analyzer", "Redirection", 0) == 1) {
    isDeviceConnected = true;
    klee_message("[RealInterrupt] Forwarding option ON");
  } else {
    isDeviceConnected = false;
    klee_message("[RealInterrupt] Forwarding option OFF");
  }

  if (isDeviceConnected) {
    // configure trace
    if (RealTarget::inception_device == NULL)
      RealTarget::inception_device = jtag_init();

    Watcher watcher = &RealInterrupt::raise;
    trace_init(Inception::RealTarget::inception_device, watcher);
  }

  // lookup in the symbol table to find the base of the interrupt vector
  if (Configurator::getAsInteger("Analyzer", "DynamicInterruptTable", 1) == 1) {
    isDynamicInterruptTable = true;
    klee_message("[RealInterrupt] Dynamic interrupt table option ON");
  } else {
    isDynamicInterruptTable = false;
    klee_message("[RealInterrupt] Dynamic interrupt table option OFF, using "
                 "the static one in the configuration file");
  }
  Module *m = RealInterrupt::executor->getKModule()->module;
  Inception::SymbolsTable *ST = new Inception::SymbolsTable(m);
  Inception::SymbolInfo *Info;
  Info = ST->lookUpVariable(".interrupt_vector");
  if (Info == NULL) {
    klee_warning("[RealInterrupt] .interrupt_vector not found, error if you "
                 "are using interrupts");
    RealInterrupt::interrupt_vector_base_addr = 0;
  } else {
    RealInterrupt::interrupt_vector_base_addr = Info->base;
  }

  // HACK: eat interrupts that may still be in the buffer of the fpga due to
  // wrong
  // reset and that arrived here at the very beginning of the initialization.
  // This is a trick, of course it would be better to add a proper reset to all
  // boards.
  while (!RealInterrupt::pending_interrupts.empty()) {
    klee_warning("[RealInterrupt::init] eating old interrupts in the buffer");
    RealInterrupt::pending_interrupts.pop();
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
 * INJECTING A CALL TO THE HANDLER
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
 * control flow instruction that modifies pc...
 *
 * INTERRUPT VECTOR TABLE
 * There are two ways to find the right handler to execute given an interrupt ID
 * a) DynamicInterruptTable = 1 (Analyzer option in the json config file)
 *    We call the inception_interrupt_handler( address = table[base + id * 4] ),
 *    we read base form the .elf file, the table is allocated and initialised by
 *    Compiler.
 *    The inception_interrupt_handler function, created by Compiler, does 3
 *    things:
 *    1. save the context
 *    2. call icp(address) (indirect call to the handler transformed into a
 *       direct call)
 *    3. restore the context
 * b) DynamicInterruptTable = 0
 *    We use the information in the json file to find the name of the handler
 *    given the interrupt id. We can therefore directly call the handler from
 *    here.
 *
 * CURRENT LIMITATIONS
 * 1. Interrupt nesting is not allowed
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

  // return if the caller is one klee or inception function that should be
  // atomic
  if (caller->getName().find("klee_") != std::string::npos ||
      caller->getName().find("inception_") != std::string::npos)
    return;

  // find the pc and update the PC reg
  klee::ref<klee::Expr> PC = executor->getPCAddress();
  int current_id = current->stack.getSelectedThreadID();
  klee_warning("[RealInterrupt] updating pc to current id %p", current_id);
  klee::ref<klee::Expr> ID =
      klee::ConstantExpr::create(current_id, Expr::Int32);
  executor->writeAt(*current, PC, ID);

  // get the pending interrupt
  RealInterrupt::current_interrupt = RealInterrupt::pending_interrupts.top();
  RealInterrupt::pending_interrupts.pop();

  // get the name of the interrupt handler
  llvm::StringRef function_name;
  if (RealInterrupt::isDynamicInterruptTable) {
    // in case of dynamic interrupt vector, we call a generic handler which
    // will look into the vector and find the address of the handler
    function_name = "inception_interrupt_handler";
  } else {
    // in case of static interrupt vector, we resolve the name here, thanks to
    // the static vector described in the configuration file
    function_name = RealInterrupt::current_interrupt->handlerName;
  }

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

  if (RealInterrupt::isDynamicInterruptTable) {
    // the generic handler takes as parameter the address of the interrupt
    // vector location in which to look for the handler address
    uint32_t vector_address = RealInterrupt::interrupt_vector_base_addr +
                              (RealInterrupt::current_interrupt->id << 2);
    klee::ref<klee::Expr> Vector_address =
        klee::ConstantExpr::create(vector_address, Expr::Int32);
    klee::ref<klee::Expr> Handler_address =
        executor->readAt(*current, Vector_address);

    klee::ConstantExpr *handler_address_ce =
        dyn_cast<klee::ConstantExpr>(Handler_address);
    uint32_t handler_address = handler_address_ce->getZExtValue();

    klee_message("resolving handler address: vector(%p) = %p", vector_address,
                 handler_address);

    Cell &argumentCell = current->stack.back().locals[kf->getArgRegister(0)];
    argumentCell.value = Handler_address;
  }

  // finally "call" the handler by setting the pc to point to it
  current->pc = kf->instructions;

  // flag to mark the interrupted state
  // it is useful in the current version to forbid nested interrupts
  RealInterrupt::interrupted = true;
}

void RealInterrupt::enable() { enabled = true; }

void RealInterrupt::disable() { enabled = false; }

void RealInterrupt::write_basepri(klee::ref<klee::Expr> basepri) {
  if (isDeviceConnected) {
    jtag_halt(RealTarget::inception_device);
    uint32_t old_20 = jtag_read_reg(RealTarget::inception_device, 20);
    uint32_t new_20 = dyn_cast<klee::ConstantExpr>(basepri)->getZExtValue();
    klee_message("[write_basepri] writing %p to basepri\n", new_20);
    new_20 = ((new_20 & 0xff) << 8) | (old_20 & ~0xff00);
    jtag_write_reg(RealTarget::inception_device, 20, new_20);
    jtag_resume(RealTarget::inception_device);
  }
}

void RealInterrupt::read_basepri(klee::ref<klee::Expr> basepri_ptr) {
  if (isDeviceConnected) {
    jtag_halt(RealTarget::inception_device);
    uint32_t basepri = jtag_read_reg(RealTarget::inception_device, 20);
    basepri = basepri >> 8;
    basepri &= 0xff;
    klee_message("[read_basepri] reading %p from basepri\n", basepri);
    jtag_resume(RealTarget::inception_device);
    klee::ref<klee::Expr> Basepri =
        klee::ConstantExpr::create(basepri, Expr::Int32);

    ExecutionState *current = RealInterrupt::executor->getExecutionState();
    executor->writeAt(*current, basepri_ptr, Basepri);
  }
}

void RealInterrupt::is_irq(klee::ref<klee::Expr> interrupted_ptr) {
  if (isDeviceConnected) {
    int ret = 0;
    if (RealInterrupt::interrupted) {
      ret = RealInterrupt::current_interrupt->id;
    }
    klee::ref<klee::Expr> Interrupted =
        klee::ConstantExpr::create(ret, Expr::Int32);

    ExecutionState *current = RealInterrupt::executor->getExecutionState();
    executor->writeAt(*current, interrupted_ptr, Interrupted);
  }
}

} // namespace Inception
