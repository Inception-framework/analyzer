#include "inception/StackManager.h"

#include "klee/ExecutionState.h"

namespace Inception {

StackManager::StackManager(): SelectedThreadID(0xFFFFFFFF) {}

StackManager::~StackManager() {}

/*
 * Return last pushed StackFrame of current Thread
 */
klee::StackFrame &StackManager::back() {

  return threads.find(SelectedThreadID)->second.back();
}

/*Ok
 * Return last pushed StackFrame of current Thread
 */
void StackManager::push_back(klee::StackFrame sf) {

  if(threads.empty()) {
    stack_ty stack;
    stack.push_back(sf);

    threads.insert(std::pair<uint64_t,stack_ty>(SelectedThreadID, stack));
  } else {
    threads.find(SelectedThreadID)->second.push_back(sf);
  }
}

/*
 * Pop last pushed StackFrame of current Thread
 */
void StackManager::pop_back() {

  threads.find(SelectedThreadID)->second.pop_back();
}

/*
 * Return number of StackFrame for current Thread
 */
int StackManager::size() {

  return threads.find(SelectedThreadID)->second.size();
}

/*
 * Return StackFrame at index i of current Thread
 */
klee::StackFrame StackManager::at(int i) {

  return threads.find(SelectedThreadID)->second.at(i);
}

/*
 * True if current Thread has no StackFrame
 */
bool StackManager::empty() {

  return threads.find(SelectedThreadID)->second.empty();
}

/*
 * Return reverse iterator for StackFrames list of current Thread
 */
std::vector<klee::StackFrame>::reverse_iterator StackManager::rbegin() {

  return threads.find(SelectedThreadID)->second.rbegin();
}

std::vector<klee::StackFrame>::const_reverse_iterator
StackManager::rbegin() const {

  return threads.find(SelectedThreadID)->second.rbegin();
}

/*
 * Return reverse iterator end for StackFrames list of current Thread
 */
std::vector<klee::StackFrame>::reverse_iterator StackManager::rend() {

  return threads.find(SelectedThreadID)->second.rend();
}

std::vector<klee::StackFrame>::const_reverse_iterator
StackManager::rend() const {

  return threads.find(SelectedThreadID)->second.rend();
}

/*
 * Return iterator for StackFrames list of current Thread
 */
std::vector<klee::StackFrame>::const_iterator StackManager::begin() const {

  return threads.find(SelectedThreadID)->second.begin();
}

/*
 * Return iterator for StackFrames list of current Thread
 */
std::vector<klee::StackFrame>::iterator StackManager::begin() {

  return threads.find(SelectedThreadID)->second.begin();
}

/*
 * Return iterator end for StackFrames list of current Thread
 */
std::vector<klee::StackFrame>::const_iterator StackManager::end() const {

  return threads.find(SelectedThreadID)->second.end();
}

/*
 * Return iterator end for StackFrames list of current Thread
 */
std::vector<klee::StackFrame>::iterator StackManager::end() {

  return threads.find(SelectedThreadID)->second.end();
}

} // namespace Inception
