#ifndef STACK_MANAGER_H
#define STACK_MANAGER_H

#include <map>
#include <vector>

namespace klee {
class StackFrame;
} // namespace klee

namespace Inception {

class StackManager {

  typedef std::vector<klee::StackFrame> stack_ty;

public:
  StackManager();
  ~StackManager();

  /*
   * Return last pushed StackFrame of current Thread
   */
  klee::StackFrame &back();

  /*
   * Insert a StackFrame into the list of current Thread
   */
  void push_back(klee::StackFrame sf);

  /*
   * Pop last pushed StackFrame of current Thread
   */
  void pop_back();

  /*
   * Return last pushed StackFrame of current Thread
   */
  klee::StackFrame at(int i);

  /*
   * Return number of StackFrame for current Thread
   */
  int size();

  /*
   * True if current Thread has no StackFrame
   */
  bool empty();

  /*
   * Return reverse iterator for StackFrames list of current Thread
   */
  std::vector<klee::StackFrame>::reverse_iterator rbegin();
  std::vector<klee::StackFrame>::const_reverse_iterator rbegin() const;

  /*
   * Return reverse iterator end for StackFrames list of current Thread
   */
  std::vector<klee::StackFrame>::reverse_iterator rend();
  std::vector<klee::StackFrame>::const_reverse_iterator rend() const;

  /*
   * Return iterator for StackFrames list of current Thread
   */
  std::vector<klee::StackFrame>::const_iterator begin() const;
  std::vector<klee::StackFrame>::iterator begin();

  klee::StackFrame &operator[](int n) {
    return threads.find(SelectedThreadID)->second[n];
  }

  const klee::StackFrame &operator[](int n) const {
    return threads.find(SelectedThreadID)->second[n];
  }

  /*
   * Return iterator end for StackFrames list of current Thread
   */
  std::vector<klee::StackFrame>::const_iterator end() const;
  std::vector<klee::StackFrame>::iterator end();

  void switchContext(int new_id) { SelectedThreadID = new_id; }

  void setMainThreadID(int main_id) { MainThreadID = main_id; }

  bool isMainThreadID(void) { return SelectedThreadID == MainThreadID; }

private:
  /*
   * This data structure maps addresses (keys) to a list of StackFrames.
   * Each executed thread has it's own, unique and seamless key.
   * This Key is managed by the executed code (see FreeRTOS PendSVHandler).
   */
  std::map<uint64_t, stack_ty> threads;

  /*
   * This is the current Thread index
   */
  unsigned SelectedThreadID;
  unsigned MainThreadID;
};

} // namespace Inception
#endif
