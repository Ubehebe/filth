#ifndef SINGLE_LOCKED_QUEUE_HPP
#define SINGLE_LOCKED_QUEUE_HPP

#include <queue>

#include "ConcurrentQueue.hpp"
#include "Locks.hpp"

// Don't use this! For testing only.
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
