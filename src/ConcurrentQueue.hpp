#ifndef CONCURRENT_QUEUE_HPP
#define CONCURRENT_QUEUE_HPP

/** \brief Abstract interface for shared queues. */
template<class T> class ConcurrentQueue
{
public:
  /** \brief Enqueue. */
  virtual void enq(T t) = 0;
  /** \brief Dequeue, blocking if the queue is empty. */
  virtual T wait_deq() = 0;
  /** \brief Dequeue, returning true if we got something
   * and false if the queue was empty. */
  virtual bool nowait_deq(T &t) = 0;
};

#endif // CONCURRENT_QUEUE_HPP
