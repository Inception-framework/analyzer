#include "inception/Monitor.h"

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

  } catch (const std::out_of_range &oor) {
  }
}

void Monitor::dump() {

  if (!Monitor::executor) {
    klee_warning("Monitor has not been initialized ...");
    return;
  }

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

    std::ofstream log_file;
    log_file.open("registers.dump", std::ios::out | std::ios::app);
    log_file << b->first << "\n";
    log_file << info;
    log_file.close();
  }
}

void Monitor::init(Executor *_executor) {

  executor = _executor;

  // CURL *curl;
  // CURLcode res;
  //
  // curl = curl_easy_init();
  // if(curl) {
  //   curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3030");
  //   /* example.com is redirected, so we tell libcurl to follow redirection */
  //   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  //
  //   /* Perform the request, res will get the return code */
  //   res = curl_easy_perform(curl);
  //   /* Check for errors */
  //   if(res != CURLE_OK)
  //     fprintf(stderr, "curl_easy_perform() failed: %s\n",
  //             curl_easy_strerror(res));
  //
  //   /* always cleanup */
  //   curl_easy_cleanup(curl);
  // }
}

// void Monitor::run() {
//
//   Monitor::running = true;
//
//   // std::thread trace_thread (&Trace::run, trace);
//   //
//   // trace_thread.detach();
//   while(running) {
//
//     char buffer[2048] = {0};
//
//     size_t write_data(&buffer, 2048, size_t nmemb, void *userp);
//   }
//
// }

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

  if (!Monitor::trace_io_on)
    return;

  klee::ConstantExpr *address_ce = dyn_cast<klee::ConstantExpr>(address);
  uint64_t concrete_address = address_ce->getZExtValue();

  klee::ConstantExpr *value_ce = dyn_cast<klee::ConstantExpr>(value);
  uint64_t concrete_value = value_ce->getZExtValue();

  std::string srcFile = target->info->file;
  if (srcFile.length() > 42)
    srcFile = srcFile.substr(42);

  std::string debug =
      std::to_string(target->info->line) + " of " + srcFile + "\n ";

  if (isWrite) {
    printf("[RealWrite] *0x%08x = 0x%08x, %s\n\n ", concrete_address,
           concrete_value, debug.c_str());
  } else {
    printf("[RealRead] *0x%08x->0x%08x, %s\n\n ", concrete_address,
           concrete_value, debug.c_str());
  }
}

} // namespace Inception
