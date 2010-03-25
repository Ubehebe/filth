#ifndef SINGLE_LOCKED_QUEUE_HPP
#define SINGLE_LOCKED_QUEUE_HPP

#include <queue>

#include "ConcurrentQueue.hpp"
#include "Locks.hpp"

/* This queue is simple but not very concurrent. It is correct.
 * In case you are seeing bugs that seem to originate from it,
 * here is some evidence that you are wrong: it's worked
 * correctly at least 3330 times in a row. The test was to set up
 * a random number of producers (between 1 and 50), a random number
 * of consumers (between 1 and 50), and a random number of items
 * per producer (between 1 and 10,000), and start them all at about
 * the same time. After everything is done, we check that every item
 * enqueued was dequeued exactly once. */
template<class T> class SingleLockedQueue : public ConcurrentQueue<T>
{
  SingleLockedQueue(SingleLockedQueue const &);
  SingleLockedQueue &operator=(SingleLockedQueue const &);

  std::queue<T> q;
  Mutex m;
  CondVar nonempty;
public:
  SingleLockedQueue() : nonempty(m) {}
  void enq(T t);
  T wait_deq();
  bool nowait_deq(T &t);
};

template<class T> void SingleLockedQueue<T>::enq(T t)
{
  m.lock();
  bool dosignal = q.empty();
  q.push(t);
  if (dosignal)
    nonempty.signal();
  m.unlock();
}

template<class T> T SingleLockedQueue<T>::wait_deq()
{
  m.lock();
  while (q.empty())
    nonempty.wait();
  T ans = q.front();
  q.pop();
  m.unlock();
  return ans;
}

template<class T> bool SingleLockedQueue<T>::nowait_deq(T &t)
{
  m.lock();
  if (q.empty()) {
    m.unlock();
    return false;
  }
  else {
    t = q.front();
    q.pop();
    m.unlock();
    return true;
  }
}

#endif // SINGLE_LOCKED_QUEUE_HPP
