// #include "klee/InstructionsExecutor.h"
//
// #include "llvm/IR/CallSite.h"
//
// void InstructionsExecutor::select() {
//
//    ref<Expr> cond = eval(ki, 0, state).value;
//    ref<Expr> tExpr = eval(ki, 1, state).value;
//    ref<Expr> fExpr = eval(ki, 2, state).value;
//    ref<Expr> result = SelectExpr::create(cond, tExpr, fExpr);
//    bindLocal(ki, state, result);
// }
//
// void InstructionsExecutor::phi() {
//
//   #if LLVM_VERSION_CODE >= LLVM_VERSION(3, 0)
//       ref<Expr> result = eval(ki, state.incomingBBIndex, state).value;
//   #else
//       ref<Expr> result = eval(ki, state.incomingBBIndex * 2, state).value;
//   #endif
//       bindLocal(ki, state, result);
// }
//
// void InstructionsExecutor::switch () {
//   SwitchInst *si = cast<SwitchInst>(i);
//   ref<Expr> cond = eval(ki, 0, state).value;
//   BasicBlock *bb = si->getParent();
//
//   cond = toUnique(state, cond);
//   if (ConstantExpr *CE = dyn_cast<ConstantExpr>(cond)) {
//     // Somewhat gross to create these all the time, but fine till we
//     // switch to an internal rep.
//     LLVM_TYPE_Q llvm::IntegerType *Ty =
//         cast<IntegerType>(si->getCondition()->getType());
//     ConstantInt *ci = ConstantInt::get(Ty, CE->getZExtValue());
// #if LLVM_VERSION_CODE >= LLVM_VERSION(3, 1)
//     unsigned index = si->findCaseValue(ci).getSuccessorIndex();
// #else
//     unsigned index = si->findCaseValue(ci);
// #endif
//     transferToBasicBlock(si->getSuccessor(index), si->getParent(), state);
//   } else {
//     // Handle possible different branch targets
//
//     // We have the following assumptions:
//     // - each case value is mutual exclusive to all other values including the
//     //   default value
//     // - order of case branches is based on the order of the expressions of
//     //   the scase values, still default is handled last
//     std::vector<BasicBlock *> bbOrder;
//     std::map<BasicBlock *, ref<Expr> > branchTargets;
//
//     std::map<ref<Expr>, BasicBlock *> expressionOrder;
//
// // Iterate through all non-default cases and order them by expressions
// #if LLVM_VERSION_CODE >= LLVM_VERSION(3, 1)
//     for (SwitchInst::CaseIt i = si->case_begin(), e = si->case_end(); i != e;
//          ++i) {
//       ref<Expr> value = evalConstant(i.getCaseValue());
// #else
//     for (unsigned i = 1, cases = si->getNumCases(); i < cases; ++i) {
//       ref<Expr> value = evalConstant(si->getCaseValue(i));
// #endif
//
// #if LLVM_VERSION_CODE >= LLVM_VERSION(3, 1)
//       BasicBlock *caseSuccessor = i.getCaseSuccessor();
// #else
//       BasicBlock *caseSuccessor = si->getSuccessor(i);
// #endif
//       expressionOrder.insert(std::make_pair(value, caseSuccessor));
//     }
//
//     // Track default branch values
//     ref<Expr> defaultValue = ConstantExpr::alloc(1, Expr::Bool);
//
//     // iterate through all non-default cases but in order of the expressions
//     for (std::map<ref<Expr>, BasicBlock *>::iterator
//              it = expressionOrder.begin(),
//              itE = expressionOrder.end();
//          it != itE; ++it) {
//       ref<Expr> match = EqExpr::create(cond, it->first);
//
//       // Make sure that the default value does not contain this target's value
//       defaultValue = AndExpr::create(defaultValue, Expr::createIsZero(match));
//
//       // Check if control flow could take this case
//       bool result;
//       bool success = solver->mayBeTrue(state, match, result);
//       assert(success && "FIXME: Unhandled solver failure");
//       (void)success;
//       if (result) {
//         BasicBlock *caseSuccessor = it->second;
//
//         // Handle the case that a basic block might be the target of multiple
//         // switch cases.
//         // Currently we generate an expression containing all switch-case
//         // values for the same target basic block. We spare us forking too
//         // many times but we generate more complex condition expressions
//         // TODO Add option to allow to choose between those behaviors
//         std::pair<std::map<BasicBlock *, ref<Expr> >::iterator, bool> res =
//             branchTargets.insert(std::make_pair(
//                 caseSuccessor, ConstantExpr::alloc(0, Expr::Bool)));
//
//         res.first->second = OrExpr::create(match, res.first->second);
//
//         // Only add basic blocks which have not been target of a branch yet
//         if (res.second) {
//           bbOrder.push_back(caseSuccessor);
//         }
//       }
//     }
//
//     // Check if control could take the default case
//     bool res;
//     bool success = solver->mayBeTrue(state, defaultValue, res);
//     assert(success && "FIXME: Unhandled solver failure");
//     (void)success;
//     if (res) {
//       std::pair<std::map<BasicBlock *, ref<Expr> >::iterator, bool> ret =
//           branchTargets.insert(
//               std::make_pair(si->getDefaultDest(), defaultValue));
//       if (ret.second) {
//         bbOrder.push_back(si->getDefaultDest());
//       }
//     }
//
//     // Fork the current state with each state having one of the possible
//     // successors of this switch
//     std::vector<ref<Expr> > conditions;
//     for (std::vector<BasicBlock *>::iterator it = bbOrder.begin(),
//                                              ie = bbOrder.end();
//          it != ie; ++it) {
//       conditions.push_back(branchTargets[*it]);
//     }
//     std::vector<ExecutionState *> branches;
//     branch(state, conditions, branches);
//
//     std::vector<ExecutionState *>::iterator bit = branches.begin();
//     for (std::vector<BasicBlock *>::iterator it = bbOrder.begin(),
//                                              ie = bbOrder.end();
//          it != ie; ++it) {
//       ExecutionState *es = *bit;
//       if (es)
//         transferToBasicBlock(*it, bb, *es);
//       ++bit;
//     }
//   }
// }
// }
//
// void InstructionsExecutor::br() {
//
//   BranchInst *bi = cast<BranchInst>(i);
//   if (bi->isUnconditional()) {
//     transferToBasicBlock(bi->getSuccessor(0), bi->getParent(), state);
//   } else {
//     // FIXME: Find a way that we don't have this hidden dependency.
//     assert(bi->getCondition() == bi->getOperand(0) && "Wrong operand index!");
//     ref<Expr> cond = eval(ki, 0, state).value;
//     Executor::StatePair branches = fork(state, cond, false);
//
//     // NOTE: There is a hidden dependency here, markBranchVisited
//     // requires that we still be in the context of the branch
//     // instruction (it reuses its statistic id). Should be cleaned
//     // up with convenient instruction specific data.
//     if (statsTracker && state.stack.back().kf->trackCoverage)
//       statsTracker->markBranchVisited(branches.first, branches.second);
//
//     if (branches.first)
//       transferToBasicBlock(bi->getSuccessor(0), bi->getParent(),
//                            *branches.first);
//     if (branches.second)
//       transferToBasicBlock(bi->getSuccessor(1), bi->getParent(),
//                            *branches.second);
//   }
//   break;
// }
//
// void InstructionsExecutor::unwind() {
//   for (;;) {
//     KInstruction *kcaller = state.stack.back().caller;
//     state.popFrame();
//
//     if (statsTracker)
//       statsTracker->framePopped(state);
//
//     if (state.stack.empty()) {
//       terminateStateOnExecError(state, "unwind from initial stack frame");
//       break;
//     } else {
//       Instruction *caller = kcaller->inst;
//       if (InvokeInst *ii = dyn_cast<InvokeInst>(caller)) {
//         transferToBasicBlock(ii->getUnwindDest(), caller->getParent(), state);
//         break;
//       }
//     }
//   }
//   break;
// }
//
// void InstructionsExecutor::ret() {
//
//   ReturnInst *ri = cast<ReturnInst>(i);
//   KInstIterator kcaller = state.stack.back().caller;
//   Instruction *caller = kcaller ? kcaller->inst : 0;
//   bool isVoidReturn = (ri->getNumOperands() == 0);
//   ref<Expr> result = ConstantExpr::alloc(0, Expr::Bool);
//
//   if (!isVoidReturn) {
//     result = eval(ki, 0, state).value;
//   }
//
//   if (!state.interrupted && state.stack.size() <= 1) {
//     assert(!caller && "caller set on initial stack frame");
//     terminateStateOnExit(state);
//   } else {
//     state.popFrame();
//
//     if (statsTracker)
//       statsTracker->framePopped(state);
//
//     if (InvokeInst *ii = dyn_cast<InvokeInst>(caller)) {
//       transferToBasicBlock(ii->getNormalDest(), caller->getParent(), state);
//     } else {
//       state.pc = kcaller;
//       ++state.pc;
//     }
//
//     if (!isVoidReturn) {
//       LLVM_TYPE_Q Type *t = caller->getType();
//       if (t != Type::getVoidTy(getGlobalContext())) {
//         // may need to do coercion due to bitcasts
//         Expr::Width from = result->getWidth();
//         Expr::Width to = getWidthForLLVMType(t);
//
//         if (from != to) {
//           CallSite cs =
//               (isa<InvokeInst>(caller) ? CallSite(cast<InvokeInst>(caller))
//                                        : CallSite(cast<CallInst>(caller)));
//
// // XXX need to check other param attrs ?
// #if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
//           bool isSExt = cs.paramHasAttr(0, llvm::Attribute::SExt);
// #elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 2)
//           bool isSExt = cs.paramHasAttr(0, llvm::Attributes::SExt);
// #else
//           bool isSExt = cs.paramHasAttr(0, llvm::Attribute::SExt);
// #endif
//           if (isSExt) {
//             result = SExtExpr::create(result, to);
//           } else {
//             result = ZExtExpr::create(result, to);
//           }
//         }
//
//         bindLocal(kcaller, state, result);
//       }
//     } else {
//       // We check that the return value has no users instead of
//       // checking the type, since C defaults to returning int for
//       // undeclared functions.
//       if (!caller->use_empty()) {
//         terminateStateOnExecError(state,
//                                   "return void when caller expected a result");
//       }
//     }
//   }
//   break;
// }
//
// void InstructionsExecutor::call(klee::ExecutionState &state,
//                                 klee::KInstruction *ki) {
//   CallSite cs(i);
//
//   unsigned numArgs = cs.arg_size();
//   Value *fp = cs.getCalledValue();
//   Function *f = getTargetFunction(fp, state);
//
//   // Skip debug intrinsics, we can't evaluate their metadata arguments.
//   if (f && isDebugIntrinsic(f, kmodule))
//     break;
//
//   if (isa<InlineAsm>(fp)) {
//
//     CallInst *asm_instruction = dyn_cast<CallInst>(i);
//
//     Value *called_vallue = asm_instruction->getCalledValue();
//
//     InlineAsm *assembly = dyn_cast<InlineAsm>(called_vallue);
//
//     if (!Inception::AsmJIT::parse_block(assembly, state, ki))
//       terminateStateOnExecError(state, "inline assembly is unsupported");
//
//     // klee_warning("Unsupported instruction : %s %s",i->getOpcodeName(),
//     // f->getName().str());
//
//     break;
//   }
//   // evaluate arguments
//   std::vector<ref<Expr> > arguments;
//   arguments.reserve(numArgs);
//
//   for (unsigned j = 0; j < numArgs; ++j)
//     arguments.push_back(eval(ki, j + 1, state).value);
//
//   if (f) {
//     const FunctionType *fType = dyn_cast<FunctionType>(
//         cast<PointerType>(f->getType())->getElementType());
//     const FunctionType *fpType = dyn_cast<FunctionType>(
//         cast<PointerType>(fp->getType())->getElementType());
//
//     // special case the call with a bitcast case
//     if (fType != fpType) {
//       assert(fType && fpType && "unable to get function type");
//
//       // XXX check result coercion
//
//       // XXX this really needs thought and validation
//       unsigned i = 0;
//       for (std::vector<ref<Expr> >::iterator ai = arguments.begin(),
//                                              ie = arguments.end();
//            ai != ie; ++ai) {
//         Expr::Width to, from = (*ai)->getWidth();
//
//         if (i < fType->getNumParams()) {
//           to = getWidthForLLVMType(fType->getParamType(i));
//
//           if (from != to) {
// // XXX need to check other param attrs ?
// #if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
//             bool isSExt = cs.paramHasAttr(i + 1, llvm::Attribute::SExt);
// #elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 2)
//             bool isSExt = cs.paramHasAttr(i + 1, llvm::Attributes::SExt);
// #else
//             bool isSExt = cs.paramHasAttr(i + 1, llvm::Attribute::SExt);
// #endif
//             if (isSExt) {
//               arguments[i] = SExtExpr::create(arguments[i], to);
//             } else {
//               arguments[i] = ZExtExpr::create(arguments[i], to);
//             }
//           }
//         }
//
//         i++;
//       }
//     }
//
//     executeCall(state, ki, f, arguments);
//   } else {
//     ref<Expr> v = eval(ki, 0, state).value;
//
//     ExecutionState *free = &state;
//     bool hasInvalid = false, first = true;
//
//     /* XXX This is wasteful, no need to do a full evaluate since we
//        have already got a value. But in the end the caches should
//        handle it for us, albeit with some overhead. */
//     do {
//       ref<ConstantExpr> value;
//       bool success = solver->getValue(*free, v, value);
//       assert(success && "FIXME: Unhandled solver failure");
//       (void)success;
//       StatePair res = fork(*free, EqExpr::create(v, value), true);
//       if (res.first) {
//         uint64_t addr = value->getZExtValue();
//         if (legalFunctions.count(addr)) {
//           f = (Function *)addr;
//
//           // Don't give warning on unique resolution
//           if (res.second || !first)
//             klee_warning_once((void *)(unsigned long)addr,
//                               "resolved symbolic function pointer to: %s",
//                               f->getName().data());
//
//           executeCall(*res.first, ki, f, arguments);
//         } else {
//           if (!hasInvalid) {
//             terminateStateOnExecError(state, "invalid function pointer");
//             hasInvalid = true;
//           }
//         }
//       }
//
//       first = false;
//       free = res.second;
//     } while (free);
//   }
//   break;
// }
// }
