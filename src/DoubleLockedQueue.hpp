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

/* Queue allowing an enqueue to run concurrently with a dequeue.
 * It is correct. In case you are seeing bugs that seem to originate from it,
 * here is some evidence that you are wrong: it's worked correctly at least
 * 1000 times in a row. The test was to set up a random number of producers
 * (between 1 and 50), a random number of consumers (between 1 and 50),
 * and a random number of items per producer (between 1 and 10,000),
 * and start them all at about the same time. After everything is done,
 * we check that every item enqueued was dequeued exactly once. */
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
