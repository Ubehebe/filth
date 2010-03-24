#ifndef CONCURRENT_QUEUE_HPP
#define CONCURRENT_QUEUE_HPP

template<class T> class ConcurrentQueue
{
public:
  virtual void enq(T t) = 0;
  virtual T wait_deq() = 0;
  virtual bool nowait_deq(T &t) = 0;
};

#endif // CONCURRENT_QUEUE_HPP
