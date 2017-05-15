#include "klee/RealInterrupt.h"

namespace Inception{

std::map<uint32_t, Interrupt*> RealInterrupt::interrupts_vector;

std::priority_queue<Interrupt*, std::vector<Interrupt*>, InterruptComparator> RealInterrupt::pending_interrupts;

RealInterrupt::RealInterrupt(){

  AddInterrupt(StringRef("__cs3_reset"), 0, 0, 0);                 /* Reset Handler                */
  AddInterrupt(StringRef("NMI_Handler"), 1, 0, 0);                 /* NMI Handler                  */
  AddInterrupt(StringRef("HardFault_Handler"), 2, 0, 0);           /* Hard Fault Handler           */
  AddInterrupt(StringRef("MemManage_Handler"), 3, 0, 0);           /* MPU Fault Handler            */
  AddInterrupt(StringRef("BusFault_Handler"), 4, 0, 0);            /* Bus Fault Handler            */
  AddInterrupt(StringRef("UsageFault_Handler"), 5, 0, 0);          /* Usage Fault Handler          */
  AddInterrupt(StringRef("Sign_Value"), 6, 0, 0);                  /* Reserved                     */
  AddInterrupt(StringRef(""), 7, 0, 0);                           /* Reserved                     */
  AddInterrupt(StringRef(""), 8, 0, 0);                           /* Reserved                     */
  AddInterrupt(StringRef(""), 9, 0, 0);                           /* Reserved                     */
  AddInterrupt(StringRef("SVC_Handler"), 10, 0, 0);                 /* SVCall Handler               */
  AddInterrupt(StringRef("DebugMon_Handler"), 11, 0, 0);            /* Debug Monitor Handler        */
  AddInterrupt(StringRef(""), 12, 0, 0);                         /* Reserved                     */
  AddInterrupt(StringRef("PendSV_Handler"), 13, 0, 0);              /* PendSV Handler               */
  AddInterrupt(StringRef("SysTick_Handler"), 14, 0, 0);             /* SysTick Handler              */
}

void RealInterrupt::AddInterrupt(StringRef handler_name, uint32_t id, uint32_t group_priority, uint32_t internal_priority) {

  interrupts_vector.insert(std::pair<uint32_t, Interrupt*>(id, new Interrupt(handler_name, id, group_priority, internal_priority)));
}

void RealInterrupt::raise(uint32_t id) {

  /*Reorganized pending queue to */
  Interrupt* interrupt = interrupts_vector.at(id);

  pending_interrupts.push(interrupt);
}

RealInterrupt::~RealInterrupt(){}

bool RealInterrupt::is_up() {
  return false;
}

Function* RealInterrupt::next_int_function() {
  return NULL;
}

}
