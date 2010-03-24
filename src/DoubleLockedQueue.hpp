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
#include "logging.h"

/* I experience deadlock rather a lot on testing. The most likely
 * explanation is it's incorrect, but sometimes I also experience deadlock with
 * a single locked queue, i.e. one that just wraps all its services in a single
 * lock. I don't understand how deadlock could occur there, so perhaps
 * there is an issue with pthreads I don't understand. */

/* Simple locked queue, with a lock for the front and a lock for the back.
 * The interface is a bit different from the STL interfaces; in particular,
 * since I don't know enough about the invalidation of iterators,
 * we don't expose iterators at all. */
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
  if (dosignal) { // Lost signal?
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
