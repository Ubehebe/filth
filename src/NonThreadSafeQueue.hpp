#ifndef NON_THREAD_SAFE_QUEUE_HPP
#define NON_THREAD_SAFE_QUEUE_HPP

#include <queue>

#include "ConcurrentQueue.hpp"
#include "Locks.hpp"

/* This just connects the STL queue to my ConcurrentQueue interface,
 * so we can use it in tests and demonstrate that it is in fact not
 * thread-safe. This should be its only use. */
template<class T> class NonThreadSafeQueue : public ConcurrentQueue<T>
{
  NonThreadSafeQueue(NonThreadSafeQueue const &);
  NonThreadSafeQueue &operator=(NonThreadSafeQueue const &);

  std::queue<T> q;

  /* The semaphore is just used to sleep in wait_deq, it does not provide
   * any kind of synchronization--that's the point. */
  Semaphore sem;
public:
  NonThreadSafeQueue() {}
  void enq(T t);
  T wait_deq();
  bool nowait_deq(T &t);
};

template<class T> void NonThreadSafeQueue<T>::enq(T t)
{
  q.push(t);
  sem.up();
}

template<class T> T NonThreadSafeQueue<T>::wait_deq()
{
  sem.down();
  T ans = q.front();
  q.pop();
  return ans;
}

template<class T> bool NonThreadSafeQueue<T>::nowait_deq(T &t)
{
  if (q.empty()) {
    return false;
  }
  else {
    sem.down();
    t = q.front();
    q.pop();
    return true;
  }
}

#endif // NON_THREAD_SAFE_QUEUE_HPP
