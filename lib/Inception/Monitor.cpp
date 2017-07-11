#include "inception/Monitor.h"

#include <fstream>

// #include <thread>
// #include <curl/curl.h>
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/Support/ErrorHandling.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

namespace Inception {

bool Monitor::running = false;

klee::Executor* Monitor::executor = NULL;

std::map<std::string, uint64_t> Monitor::followed = {
  {std::pair<std::string, uint64_t>("R0",0)},
  {std::pair<std::string, uint64_t>("R1",0)},
  {std::pair<std::string, uint64_t>("R2",0)},
  {std::pair<std::string, uint64_t>("R3",0)},
  {std::pair<std::string, uint64_t>("R4",0)},
  {std::pair<std::string, uint64_t>("R5",0)},
  {std::pair<std::string, uint64_t>("R6",0)},
  {std::pair<std::string, uint64_t>("R7",0)},
  {std::pair<std::string, uint64_t>("R8",0)},
  {std::pair<std::string, uint64_t>("R9",0)},
  {std::pair<std::string, uint64_t>("R10",0)},
  {std::pair<std::string, uint64_t>("R11",0)},
  {std::pair<std::string, uint64_t>("R12",0)},
  {std::pair<std::string, uint64_t>("LR",0)},
  {std::pair<std::string, uint64_t>("SP",0)},
  {std::pair<std::string, uint64_t>("PC",0)},
  {std::pair<std::string, uint64_t>("CPSR",0)},
};

Monitor::Monitor() {}

Monitor::~Monitor() {}

void Monitor::follow(const llvm::GlobalVariable* i, uint64_t address) {

  try{

    // llvm::errs() << "Should I follow : " << i->getName() << "\n";

    std::map<std::string, uint64_t>::iterator it = Monitor::followed.find(i->getName());
    if (it != Monitor::followed.end())
        it->second = address;
    else
      return;

    // klee_warning("Monitor is watching one register more...");

  }
  catch (const std::out_of_range& oor) {
  }
}

void Monitor::dump() {

  if(!Monitor::executor) {
    klee_warning("Monitor has not been initialized ...");
    return;
  }

  // uint64_t address = Monitor::followed.at(i->getName());
  for(auto b=Monitor::followed.begin(), e=Monitor::followed.end(); e!=b; b++) {

    if(b->second == 0) {
      // llvm::errs() << "[Monitor]\n\t Register " << b->first << " is not followed \n";
      continue;
    }

    ExecutionState* state = executor->getExecutionState();

    ref<Expr> address_ce = ConstantExpr::create(b->second, Expr::Int32);

    std::string info = executor->getAddressInfo(*state, address_ce);

    std::ofstream log_file;
    log_file.open ("registers.dump", std::ios::out | std::ios::app);
    log_file << b->first << "\n";
    log_file << info;
    log_file.close();
  }
}

void Monitor::init(Executor* _executor) {

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

}
