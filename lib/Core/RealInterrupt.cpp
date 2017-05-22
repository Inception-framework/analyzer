#include "klee/RealInterrupt.h"
#include "klee/RealTarget.h"

namespace Inception{

bool RealInterrupt::interrupted = false;

std::map<uint32_t, Interrupt*> RealInterrupt::interrupts_vector;

std::priority_queue<Interrupt*, std::vector<Interrupt*>, InterruptComparator> RealInterrupt::pending_interrupts;

RealInterrupt::RealInterrupt(){}

void RealInterrupt::init() {

  AddInterrupt(StringRef("GPIO0_IRQHandler"), 48, 0, 0);                 /* GINT0_IRQHandler                */
  AddInterrupt(StringRef("GINT0_IRQHandler"), 56, 0, 0);                 /* GINT0_IRQHandler                */
  AddInterrupt(StringRef("GINT1_IRQHandler"), 57, 0, 0);                 /* GINT1_IRQHandler                */

  // AddInterrupt(StringRef("__cs3_reset"), 0, 0, 0);                 /* Reset Handler                */
  // AddInterrupt(StringRef("NMI_Handler"), 1, 0, 0);                 /* NMI Handler                  */
  // AddInterrupt(StringRef("HardFault_Handler"), 2, 0, 0);           /* Hard Fault Handler           */
  // AddInterrupt(StringRef("MemManage_Handler"), 3, 0, 0);           /* MPU Fault Handler            */
  // AddInterrupt(StringRef("BusFault_Handler"), 4, 0, 0);            /* Bus Fault Handler            */
  // AddInterrupt(StringRef("UsageFault_Handler"), 5, 0, 0);          /* Usage Fault Handler          */
  // AddInterrupt(StringRef("Sign_Value"), 6, 0, 0);                  /* Reserved                     */
  // AddInterrupt(StringRef(""), 7, 0, 0);                           /* Reserved                     */
  // AddInterrupt(StringRef(""), 8, 0, 0);                           /* Reserved                     */
  // AddInterrupt(StringRef(""), 9, 0, 0);                           /* Reserved                     */
  // AddInterrupt(StringRef("SVC_Handler"), 10, 0, 0);                 /* SVCall Handler               */
  // AddInterrupt(StringRef("DebugMon_Handler"), 11, 0, 0);            /* Debug Monitor Handler        */
  // AddInterrupt(StringRef(""), 12, 0, 0);                         /* Reserved                     */
  // AddInterrupt(StringRef("PendSV_Handler"), 13, 0, 0);              /* PendSV Handler               */
  // AddInterrupt(StringRef("SysTick_Handler"), 14, 0, 0);             /* SysTick Handler              */

  //configure trace
  if(RealTarget::inception_device == NULL)
    RealTarget::inception_device = jtag_init();

  Watcher watcher = &RealInterrupt::raise;
  trace_init(Inception::RealTarget::inception_device, watcher);
}

void RealInterrupt::AddInterrupt(StringRef handler_name, uint32_t id, uint32_t group_priority, uint32_t internal_priority) {

  interrupts_vector.insert(std::pair<uint32_t, Interrupt*>(id, new Interrupt(handler_name, id, group_priority, internal_priority)));
}

void RealInterrupt::raise(int id) {

  /*Reorganized pending queue to */
  Interrupt* interrupt = interrupts_vector.at(id);

  llvm::errs() << "[RealInterrupt] Raise interrupt id : " << id << "\n";

  pending_interrupts.push(interrupt);
}

RealInterrupt::~RealInterrupt(){}

bool RealInterrupt::is_up() {
  if(RealInterrupt::pending_interrupts.empty())
    return false;
  else {
    llvm::errs() << "[RealInterrupt] Interrupt available !" << "\n";
    return true;
  }
}

void RealInterrupt::stop_interrupt() {

  RealInterrupt::interrupted = false;
}

bool RealInterrupt::is_interrupted() {

  return RealInterrupt::interrupted;
}

llvm::StringRef& RealInterrupt::next_int_function() {

  RealInterrupt::interrupted = true;

  Interrupt *interrupt = RealInterrupt::pending_interrupts.top();

  RealInterrupt::pending_interrupts.pop();

  return interrupt->handlerName;
}

}
