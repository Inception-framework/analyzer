#include "inception/Monitor.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>

// #include <thread>
// #include <curl/curl.h>
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/Support/ErrorHandling.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

using namespace llvm;

namespace Inception {

bool Monitor::trace_on = false;

bool Monitor::trace_io_on = false;

bool Monitor::running = false;

klee::Executor *Monitor::executor = NULL;

std::map<std::string, uint64_t> Monitor::followed = {
    {std::pair<std::string, uint64_t>("R0", 0)},
    {std::pair<std::string, uint64_t>("R1", 0)},
    {std::pair<std::string, uint64_t>("R2", 0)},
    {std::pair<std::string, uint64_t>("R3", 0)},
    {std::pair<std::string, uint64_t>("R4", 0)},
    {std::pair<std::string, uint64_t>("R5", 0)},
    {std::pair<std::string, uint64_t>("R6", 0)},
    {std::pair<std::string, uint64_t>("R7", 0)},
    {std::pair<std::string, uint64_t>("R8", 0)},
    {std::pair<std::string, uint64_t>("R9", 0)},
    {std::pair<std::string, uint64_t>("R10", 0)},
    {std::pair<std::string, uint64_t>("R11", 0)},
    {std::pair<std::string, uint64_t>("R12", 0)},
    {std::pair<std::string, uint64_t>("LR", 0)},
    {std::pair<std::string, uint64_t>("SP", 0)},
    {std::pair<std::string, uint64_t>("PC", 0)},
    {std::pair<std::string, uint64_t>("NF", 0)},
    {std::pair<std::string, uint64_t>("ZF", 0)},
    {std::pair<std::string, uint64_t>("CF", 0)},
    {std::pair<std::string, uint64_t>("VF", 0)},
    {std::pair<std::string, uint64_t>(".stack", 0)},
};

Monitor::Monitor() {}

Monitor::~Monitor() {}

void Monitor::follow(const llvm::GlobalVariable *i, uint64_t address) {

  try {

    // llvm::errs() << "Should I follow : " << i->getName() << "\n";

    std::map<std::string, uint64_t>::iterator it =
        Monitor::followed.find(i->getName());
    if (it != Monitor::followed.end())
      it->second = address;
    else
      return;

    // klee_warning("Monitor is watching one register more...");
    klee_message("[Monitor] adding %s %p", i->getName().str().c_str(), address);

  } catch (const std::out_of_range &oor) {
  }
}

std::string Monitor::dump(llvm::Function *function, llvm::Instruction *inst) {
  std::error_code ErrorInfo;

  std::string str;
  llvm::raw_string_ostream info(str);

  info << *function;

  if (ErrorInfo) {
    klee_error("Cannot save result into function.dump");
  }

  return "F\n\r" + info.str();
}

std::string Monitor::dump() {

  if (!Monitor::executor) {
    klee_warning("Monitor has not been initialized ...");
    return "";
  }

  std::string all_info = "R\n\r";
  std::ofstream log_file;
  log_file.open("registers.dump", std::ios::out | std::ios::trunc);

  // uint64_t address = Monitor::followed.at(i->getName());
  for (auto b = Monitor::followed.begin(), e = Monitor::followed.end(); e != b;
       b++) {

    if (b->second == 0) {
      // llvm::errs() << "[Monitor]\n\t Register " << b->first << " is not
      // followed \n";
      continue;
    }

    ExecutionState *state = executor->getExecutionState();

    ref<Expr> address_ce = klee::ConstantExpr::create(b->second, Expr::Int32);

    std::string info = executor->getAddressInfo(*state, address_ce);
    all_info += info;

    log_file << b->first << "\n";
    log_file << info;
  }
  log_file.close();

  return all_info;
}

void Monitor::dump_stack(int begin, int end) {

  if (!Monitor::executor) {
    klee_warning("Monitor has not been initialized ...");
    return;
  }

  std::map<std::string, uint64_t>::iterator it =
      Monitor::followed.find(".stack");

  // for (int i = begin; i < end; i++) {

  ExecutionState *state = executor->getExecutionState();

  ref<Expr> address_ce = klee::ConstantExpr::create(it->second, Expr::Int32);

  std::string info = executor->getAddressInfo(*state, address_ce, begin, end);

  std::ofstream log_file;
  log_file.open("stack.dump", std::ios::out | std::ios::app);
  // log_file << it->first << "[" << i << "]\n";
  log_file << it->first << "\n";
  log_file << info;
  log_file.close();
  //}
}

void Monitor::init(Executor *_executor) { executor = _executor; }

void Monitor::trace(ExecutionState &state, KInstruction *ki) {
  Instruction *i = ki->inst;

  if (!Monitor::trace_on)
    return;

  StringRef FctName = i->getParent()->getParent()->getName();

  std::string srcFile = ki->info->file;
  if (srcFile.length() > 42)
    srcFile = srcFile.substr(42);

  std::string debug = std::to_string(ki->info->line) + " of " + srcFile + "\n";

  llvm::errs() << "[Inception]\tinstruction: " << *i << " <-> function "
               << FctName << "\n";

  llvm::errs() << "\t(src line: " << ki->info->line << " of " << srcFile
               << "\n";

  std::vector<StackFrame>::iterator stackSeek = state.stack.begin();
  std::vector<StackFrame>::iterator stackEnd = state.stack.end();

  int stack_idx = 0;

  errs() << "asm line " << ki->info->assemblyLine << "\n";
  while (stackSeek != stackEnd) {
    errs() << "stack idx " << stack_idx << " in ";
    errs() << stackSeek->kf->function->getName();
    if (stackSeek->caller) {
      errs() << " line " << stackSeek->caller->info->assemblyLine;
      errs() << "\n";
    } else {
      errs() << " no caller\n";
    }
    ++stackSeek;
    ++stack_idx;
  }
  std::cerr << std::endl;
}

void Monitor::traceIO(ref<Expr> address, ref<Expr> value, bool isWrite,
                      KInstruction *target) {
  uint64_t concrete_value;
  uint64_t concrete_address;

  if (!Monitor::trace_io_on)
    return;

  klee::ConstantExpr *address_ce = dyn_cast<klee::ConstantExpr>(address);
  if (address_ce != NULL) {
    concrete_address = address_ce->getZExtValue();
  } else {
    printf("[TraceIO] Not a concrete address\n\n");
    return;
  }

  klee::ConstantExpr *value_ce = dyn_cast<klee::ConstantExpr>(value);
  if (value_ce != NULL) {
    concrete_value = value_ce->getZExtValue();
  } else {
    printf("[TraceIO] Not a concrete value\n\n");
    return;
  }

  std::string srcFile = target->info->file;
  if (srcFile.length() > 42)
    srcFile = srcFile.substr(42);

  std::string debug =
      std::to_string(target->info->line) + " of " + srcFile + "\n ";

  if (isWrite) {
    printf("[RealWrite] *0x%lx = 0x%lx, %s\n\n ", concrete_address,
           concrete_value, debug.c_str());
  } else {
    printf("[RealRead] *0x%lx->0x%lx, %s\n\n ", concrete_address,
           concrete_value, debug.c_str());
  }
}

} // namespace Inception
