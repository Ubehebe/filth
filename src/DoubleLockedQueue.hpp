#ifndef LOCKED_QUEUE_HPP
#define LOCKED_QUEUE_HPP

#include <errno.h>
#include <stdio.h>
#include <queue>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include "ConcurrentQueue.hpp"
#include "Locks.hpp"

/** \brief Queue allowing one concurrent enqueue + dequeue. */
template<class T> class DoubleLockedQueue : public ConcurrentQueue<T>
{
  DoubleLockedQueue(DoubleLockedQueue const&);
  DoubleLockedQueue &operator=(DoubleLockedQueue const&);

  std::queue<T> q;
  Mutex front, back;
  CondVar nonempty;
 public:
  DoubleLockedQueue() : nonempty(front) {}
  void enq(T t);
  T wait_deq();
  bool nowait_deq(T &t);
};

template<class T> void DoubleLockedQueue<T>::enq(T t)
{
  back.lock();
  bool dosignal = q.empty();
  q.push(t);
  if (dosignal) {
    /* The condition variable is associated with front, so we grab that too.
     * I am not sure if this is necessary, but it doesn't affect correctness. */
    front.lock();
    nonempty.signal();
    front.unlock();
  }
  back.unlock();
}

template<class T> T DoubleLockedQueue<T>::wait_deq()
{
  front.lock();
  while (q.empty())
    nonempty.wait();
  T ans = q.front();
  q.pop();
  front.unlock();
  return ans;
}

template<class T> bool DoubleLockedQueue<T>::nowait_deq(T &ans)
{
  front.lock();
  if (q.empty()) {
    front.unlock();
    return false;
  }
  else {
    ans = q.front();
    q.pop();
    front.unlock();
    return true;
  }
}

#endif // LOCKED_QUEUE_HPP
