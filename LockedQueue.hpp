#ifndef LOCKED_QUEUE_HPP
#define LOCKED_QUEUE_HPP

#include <errno.h>
#include <stdio.h>
#include <queue>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include "Locks.h"

template<class T> class LockedQueue
{
  std::queue<T> q;
  Mutex front, back;
  CondVar nonempty;
 public:
  LockedQueue() : nonempty(front) {}
  void enq(T t);
  T wait_deq();
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

#endif // LOCKED_QUEUE_HPP
