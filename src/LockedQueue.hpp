#ifndef LOCKED_QUEUE_HPP
#define LOCKED_QUEUE_HPP

#include <errno.h>
#include <stdio.h>
#include <queue>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include "Locks.hpp"

/* Simple locked queue, with a lock for the front and a lock for the back.
 * The interface is a bit different from the STL interfaces; in particular,
 * since I don't know enough about the invalidation of iterators,
 * we don't expose iterators at all. */
template<class T> class LockedQueue
{
  // No copying, no assigning.
  LockedQueue(LockedQueue const&);
  LockedQueue &operator=(LockedQueue const&);

  std::queue<T> q;
  Mutex front, back;
  CondVar nonempty;
 public:
  LockedQueue() : nonempty(front) {}
  void enq(T t);
  T wait_deq();
  bool nowait_deq(T &t);
};

template<class T> void LockedQueue<T>::enq(T t)
{
  back.lock();
  bool dosignal = q.empty();
  q.push(t);
  if (dosignal)
    nonempty.signal();
  back.unlock();
}

template<class T> T LockedQueue<T>::wait_deq()
{
  front.lock();
  while (q.empty())
    nonempty.wait();
  T ans = q.front();
  q.pop();
  front.unlock();
  return ans;
}

template<class T> bool LockedQueue<T>::nowait_deq(T &ans)
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
