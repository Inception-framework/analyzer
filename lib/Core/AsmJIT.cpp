#include "klee/AsmJIT.h"

#include "klee/ExecutionState.h"
#include "klee/Expr.h"
#include "klee/Internal/Module/Cell.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <klee/Expr.h>

#include "llvm/Support/CallSite.h"

#include <algorithm>
#include <iostream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string>
#include <utility>
#include <vector>

#include "klee/../../lib/Support/CRC32.cpp"

using namespace klee;
using namespace llvm;

using namespace std::placeholders;

namespace Inception {

std::map<std::string, uint32_t> CPU::create_registers() {

  std::map<std::string, uint32_t> map = {
      {std::make_pair("r0", 0)},        {std::make_pair("r1", 0)},
      {std::make_pair("r2", 0)},        {std::make_pair("r3", 0)},
      {std::make_pair("r4", 0)},        {std::make_pair("r5", 0)},
      {std::make_pair("r6", 0)},        {std::make_pair("r7", 0)},
      {std::make_pair("r8", 0)},        {std::make_pair("r9", 0)},
      {std::make_pair("r10", 0)},       {std::make_pair("r11", 0)},
      {std::make_pair("r12", 0)},

      {std::make_pair("sp", 0)},        {std::make_pair("lr", 0)},
      {std::make_pair("pc", 0)},

      {std::make_pair("psr", 0)},       {std::make_pair("primask", 0)},
      {std::make_pair("faultmask", 0)}, {std::make_pair("ipsr", 0)},

      {std::make_pair("basepri", 0)},   {std::make_pair("control", 0)},
  };

  return map;
}

std::vector<registers> CPU::create_registers_map() {
  std::vector<registers> array = {
      {CPU::create_registers()},        {CPU::create_registers()},
  };

  return array;
}

std::vector<registers> CPU::registers_map = CPU::create_registers_map();

uint32_t CPU::index = 0;

std::string CPU::supervisor_caller = "";

std::string CPU::supervisor_caller_tmp = "";

// std::map<std::string, uint32_t> CPU::registers = CPU::create_registers();

CPU::CPU() {}

CPU::~CPU() {}

// CPU* AsmJIT::cpu = new CPU();

AsmJIT::AsmJIT() {}

void AsmJIT::check_context(llvm::Instruction* caller) {

  if( caller->getParent()->getParent()->getName().str().compare(CPU::supervisor_caller) == 0) {

    CPU::index = 0;

    std::cout << "*****************************************************************" << std::endl;
    std::cout << "*****************************************************************" << std::endl;
    std::cout << "*****************************************************************" << std::endl;
    std::cout << "*****************************************************************" << std::endl;
    std::cout << "*****************************************************************" << std::endl;
    std::cout << "*****************************************************************" << std::endl;
    std::cout << "*****************************************************************" << std::endl;
    std::cout << "*****************************************************************" << std::endl;

    std::cout << "[Inception] Supervisor callback : " << CPU::supervisor_caller << std::endl;
  }
}

std::vector<klee::KInstruction *>
AsmJIT::parse_block(InlineAsm *assembly, ExecutionState &state,
                    KInstruction *target, klee::KModule *kmodule,
                    std::vector<klee::ref<klee::Expr> > arguments,
                    llvm::Instruction* i) {

  std::string instructions_block = assembly->getAsmString();

  std::cout << "[Inception] AsmJIT is processing an instructions block : "
            << instructions_block << std::endl;

  std::cout << "[Inception] AsmJIT is checking if the instruction needs to be "
               "splitted "
            << std::endl;

  std::istringstream iss(instructions_block);

  std::string token;

  std::vector<klee::KInstruction *> kinstructions;

  klee::KInstruction *created_kinstruction = 0;

  while (std::getline(iss, token, '\n'))
    if ((created_kinstruction =
         AsmJIT::parse_instruction(token, state, target, kmodule, arguments, i)) !=
        0) {
          std::cout << "Instruction forged !!!" << std::endl;
          kinstructions.push_back(created_kinstruction);
        }

  return kinstructions;
}

klee::KInstruction *
AsmJIT::parse_instruction(std::string instruction, ExecutionState &state,
                          KInstruction *target, klee::KModule *kmodule,
                          std::vector<klee::ref<klee::Expr> > arguments,
                          llvm::Instruction* i) {

  std::cout << "\tInstruction : " << instruction << std::endl;

  instruction.erase(std::remove(instruction.begin(), instruction.end(), ','),
                    instruction.end());

  std::replace(instruction.begin(), instruction.end(), '=', ' ');

  std::istringstream iss(instruction);

  std::vector<std::string> sub_parts{std::istream_iterator<std::string>{iss},
                                     std::istream_iterator<std::string>{}};

  std::for_each(sub_parts.begin(), sub_parts.end(), [](std::string &n) {
    std::cout << "Element: " << n << std::endl;
  });

  // std::bind(&AsmJIT::parse, _1, state, target));

  return parse_opcode(sub_parts, state, target, kmodule, arguments, i);
}

klee::KInstruction *
AsmJIT::parse_opcode(std::vector<std::string> sub_parts, ExecutionState &state,
                     KInstruction *target, klee::KModule *kmodule,
                     std::vector<klee::ref<klee::Expr> > arguments,
                     llvm::Instruction* i) {

  switch (crc32_rec(0xFFFFFFFF, sub_parts[0].c_str())) {
  case "MRS"_crc32:
    AsmJIT::mrs(sub_parts, state, target);
    return 0;
  case "ldr"_crc32:
    return AsmJIT::ldr(sub_parts, state, target, kmodule, arguments, i);
  case "cpsie"_crc32:
    AsmJIT::cpsie();
    return 0;
  case "cpsid"_crc32:
    AsmJIT::cpsid();
    return 0;
  case "svc"_crc32:
    AsmJIT::svc();
    return 0;
  default:
    throw std::invalid_argument("Unsupported instruction");
  }
}

bool AsmJIT::svc() {
  CPU::index = 1;

  CPU::supervisor_caller = CPU::supervisor_caller_tmp;

  std::cout << "*****************************************************************" << std::endl;
  std::cout << "*****************************************************************" << std::endl;
  std::cout << "*****************************************************************" << std::endl;
  std::cout << "*****************************************************************" << std::endl;
  std::cout << "*****************************************************************" << std::endl;
  std::cout << "*****************************************************************" << std::endl;
  std::cout << "*****************************************************************" << std::endl;
  std::cout << "*****************************************************************" << std::endl;

  std::cout << "[Inception] Supervisor call : " << CPU::supervisor_caller << std::endl;

  return true;
}

bool AsmJIT::cpsie() { return true; }

bool AsmJIT::cpsid() { return true; }

// bool AsmJIT::svc(std::vector<std::string>sub_parts, ExecutionState &state,
//                  KInstruction *target) {
//
//   return true;
// }

klee::KInstruction *AsmJIT::ldr(std::vector<std::string> sub_parts,
                                ExecutionState &state, KInstruction *target,
                                klee::KModule *kmodule,
                                std::vector<klee::ref<klee::Expr> > arguments,
                                llvm::Instruction* i) {

  if (sub_parts.size() < 3)
    return 0;

  std::cout << "\tLoad into     : " << sub_parts[1] << std::endl;
  std::cout << "\tLoaded value : " << sub_parts[2] << std::endl;

  std::map<std::string, uint32_t>::iterator it =
      CPU::registers_map[CPU::index].find(sub_parts[1]);
  if (it == CPU::registers_map[CPU::index].end()) {

    klee::klee_error("[Inception] Asm JIT doesn't support targeted register");
  } else {

    if (it->first.compare("r12") == 0) {

      // klee::klee_error("[Inception] Asm JIT is not able to call fct %s ",
      //  sub_parts[2].c_str());

      return AsmJIT::call(sub_parts[2], state, target, kmodule, arguments, i);
    } else {

      it->second = std::stoi(sub_parts[2]);
    }

    return 0;
  }

  return 0;
}

klee::KInstruction *AsmJIT::call(std::string function_name,
                                 ExecutionState &state, KInstruction *target,
                                 klee::KModule *kmodule,
                                 std::vector<klee::ref<klee::Expr> > arguments,
                                 llvm::Instruction* i) {

  CPU::supervisor_caller_tmp = function_name;

  // We need to retrieve the Function object
  llvm::Function *targetted_function =
      AsmJIT::resolveFunction(function_name, state, target, kmodule);

  IRBuilder<> builder(llvm::getGlobalContext());

  const FunctionType *fType = dyn_cast<FunctionType>(
      cast<PointerType>(targetted_function->getType())->getElementType());

  Value *call_instruction =
      builder.CreateCall(targetted_function, targetted_function->arg_begin(), "7");

  llvm::Instruction* instr = dyn_cast<llvm::Instruction>(call_instruction);

  llvm::errs() << "[Inception] Builder generate call :  \n\tinstruction: " << *instr << "\n";

  KInstruction* kinstruction = new KInstruction();
  kinstruction->inst = dyn_cast<llvm::Instruction>(call_instruction);

  CallSite cs(i);

  kinstruction->info = target->info;

  unsigned numArgs = cs.arg_size();
  kinstruction->operands = new int[numArgs+1];

  for (unsigned j=0; j<numArgs; j++)
    kinstruction->operands[j] = target->operands[j];

  kinstruction->dest = target->dest;

  // i->insertAfter (dyn_cast<llvm::Instruction>(instr));

  return kinstruction;
}

llvm::Function *AsmJIT::resolveFunction(std::string called,
                                        klee::ExecutionState &state,
                                        klee::KInstruction *target,
                                        klee::KModule *kmodule) {

  llvm::SmallPtrSet<const llvm::GlobalValue *, 3> Visited;

  Function *function = kmodule->module->getFunction(called);

  // llvm::GlobalValue gv = function->getNamedValue(called);

  return function;
  // while (true) {
  //
  //   std::string alias = state.getFnAlias(called);
  //
  //   if (alias != "") {
  //
  //     std::cout << " [*] Function " << alias << "(), alias for " <<
  //     std::endl;
  //
  //
  //     gv = currModule->getNamedValue(alias);
  //
  //     if (!gv) {
  //
  //       llvm::errs() << "Function " << alias << "(), alias for "
  //                    << old_gv->getName() << " not found!\n";
  //
  //       assert(0 && "function alias not found");
  //     }
  //
  //     if (!Visited.insert(gv))
  //       return 0;
  //   }
  //
  //   if (Function *f = dyn_cast<Function>(gv))
  //     return f;
  //   else if (GlobalAlias *ga = dyn_cast<GlobalAlias>(gv)) {
  //
  //     llvm::Constant *constant = ga->getAliasee();
  //   }
  //   else
  //     return 0;
  // }
}

klee::KInstruction *AsmJIT::mrs(std::vector<std::string> sub_parts,
                                ExecutionState &state, KInstruction *target) {

  if (sub_parts.size() < 3)
    return 0;

  std::cout << "\tMSR On     : " << sub_parts[2] << std::endl;
  std::cout << "\tSaved into : " << sub_parts[1] << std::endl;

  std::map<std::string, uint32_t>::iterator it =
      CPU::registers_map[CPU::index].find(sub_parts[2]);
  if (it == CPU::registers_map[CPU::index].end()) {

    klee::klee_error("[Inception] Asm JIT doesn't support targeted register");
  } else {

    uint32_t reg_value = it->second;

    ref<Expr> value = klee::ConstantExpr::alloc(reg_value, Expr::Int32);

    state.stack.back().locals[target->dest].value = value;

    return 0;
  }

  return 0;
}

//
// bool AsmJIT::msr(std::vector<std::string>sub_parts, ExecutionState &state,
//                  KInstruction *target) {
//
//   std::size_t found = instruction.find(",");
//
//   if (found != std::string::npos) {
//
//     std::string dest = instruction.substr(3, found);
//
//     std::cout << "Target of MSR -" << dest << "-" << std::endl;
//
//     std::map<std::string, uint32_t>::iterator it = CPU::registers_map[CPU::index].find(dest);
//     if (it == CPU::registers_map[CPU::index].end()) {
//
//       klee::klee_error("[Inception] Asm JIT doesn't support targeted
//       register");
//     } else {
//
//       ref<Expr> value = state.stack.back().locals[target->operands[0]].value;
//
//       if (klee::ConstantExpr *ce = dyn_cast<klee::ConstantExpr>(value)) {
//
//         it->second = (uint32_t)ce->getZExtValue();
//       } else {
//
//         klee::klee_error(
//             "[Inception] Asm JIT report an unsupported register value");
//       }
//
//       // uint32_t reg_value = pos->second;
//
//       return true;
//     }
//   }
//   return false;
// }
};
