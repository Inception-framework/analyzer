#ifndef ASM_JIT_H
#define ASM_JIT_H

#include "klee/ExecutionState.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"

#include "llvm/IR/InlineAsm.h"

namespace klee {
class KInstIterator;
}

namespace Inception {

typedef std::map<std::string, uint32_t> registers;

class MPU {

  // public :
  // std::vector<std::pair<MemoryRange*,Permission*>> memory_area;
};

class CPU {

public:
  MPU *mpu;

  CPU();

  ~CPU();

  static std::map<std::string, uint32_t> create_registers();

  static std::vector<registers> create_registers_map();

  static std::vector<registers> registers_map;

  static uint32_t index;

  static std::string supervisor_caller;

  static std::string supervisor_caller_tmp;

  // static std::map<std::string, uint32_t> registers;
};

class AsmJIT {

public:

  AsmJIT();

  ~AsmJIT();

  static void check_context(llvm::Instruction* caller);

  static std::vector<klee::KInstruction *>
  parse_block(llvm::InlineAsm *assembly, klee::ExecutionState &state,
              klee::KInstruction *target, klee::KModule *kmodule,
              std::vector<klee::ref<klee::Expr> > arguments,
              llvm::Instruction* i);

  static klee::KInstruction *
  parse_opcode(std::vector<std::string> sub_parts, klee::ExecutionState &state,
               klee::KInstruction *target, klee::KModule *kmodule,
               std::vector<klee::ref<klee::Expr> > arguments,
               llvm::Instruction* i);

  static klee::KInstruction *
  parse_instruction(std::string instruction, klee::ExecutionState &state,
                    klee::KInstruction *target, klee::KModule *kmodule,
                    std::vector<klee::ref<klee::Expr> > arguments,
                    llvm::Instruction* i);

  // static bool msr(std::string assembly, klee::ExecutionState &state,
  // klee::KInstruction *target);
  //
  static klee::KInstruction *mrs(std::vector<std::string> assembly,
                                 klee::ExecutionState &state,
                                 klee::KInstruction *target);

  static klee::KInstruction *
  ldr(std::vector<std::string> assembly, klee::ExecutionState &state,
      klee::KInstruction *target, klee::KModule *kmodule,
      std::vector<klee::ref<klee::Expr> > arguments, llvm::Instruction* i);

  static klee::KInstruction *
  call(std::string function_name, klee::ExecutionState &state,
       klee::KInstruction *target, klee::KModule *kmodule,
       std::vector<klee::ref<klee::Expr> > arguments, llvm::Instruction* i);

  static llvm::Function *resolveFunction(std::string called,
                                         klee::ExecutionState &state,
                                         klee::KInstruction *target,
                                         klee::KModule *kmodule);

  // static bool svc(std::string instruction, klee::ExecutionState &state,
  // klee::KInstruction *target);

  static bool svc();

  static bool cpsie();

  static bool cpsid();
};
}

#endif
