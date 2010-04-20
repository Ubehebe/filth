#ifndef LOCK_FREE_QUEUE_HPP
#define LOCK_FREE_QUEUE_HPP

#include <iostream> // just for NULL

#include "ConcurrentQueue.hpp"
#include "Locks.hpp"

/** \brief Incorrect (but rarely so) concurrent queue.
 * \remarks This is a modified version of the lock-free queue given in chapter
 * 10 of "The Art of Multiprocessor Programming". In particular, I have added a
 * semaphore in order to implement sleeping in wait_deq (their version gave only
 * a nowait_deq), and I delete the dequeued node in wait_deq and nowait_deq
 * (their version uses Java). I believe that the first change doesn't affect
 * correctness, because we maintain the invariant that the value of the
 * semaphore equals the size of the queue. The second change, however, makes
 * the queue susceptible to the ABA problem: if a node is dequeued and freed,
 * and then is recycled from the heap and put back at the same location
 * on the queue, from the perspective of a thread trying to do an atomic
 * test-and-set it appears that nothing has changed. So, this queue is not
 * actually correct. We could steal the low one or two bits from each pointer
 * to create a simple timestamp, but this would only make the ABA problem rarer,
 * not eliminate it. So, caveat emptor. You are welcome to use
 * one of the correct, locked queues. */
template<class T> class LockFreeQueue : public ConcurrentQueue<T>
{
  LockFreeQueue(LockFreeQueue const&);
  LockFreeQueue &operator=(LockFreeQueue const&);

  struct node
  {
    T t;
    node *next;
    node() : next(NULL) {}
    node(T t) : t(t), next(NULL) {}
  };
  node *head, *tail;
  Semaphore sz; // ADDITION
public:
  LockFreeQueue() : head(new node()), tail(head) {}
  ~LockFreeQueue();
  void enq(T t);
  T wait_deq(); // ADDITION
  bool nowait_deq(T &t);
};

// ADDITION. Obviously, not thread-safe.
template<class T> LockFreeQueue<T>::~LockFreeQueue()
{
  node *cur = head, *next;
  while (cur != NULL) {
    next = cur->next;
    delete cur;
    cur = next;
  }
}

template<class T> void LockFreeQueue<T>::enq(T t)
{
  node *n = new node(t);
  node *last, *next;
  while (true) {
    last = tail;
    next = last->next;
    if (tail == last) {
      if (next == NULL) {
	/* If this succeeds, we found the end of the queue and
	 * swung the new node onto the end. */
	if (__sync_bool_compare_and_swap(&last->next, NULL, n)) {
	  // Now we try to change last to point to the new end; could fail.
	  __sync_bool_compare_and_swap(&tail, last, n);
	  // ADDITION.
	  sz.up();
	  return;
	}
      }
      /* If next is not null, that means the tail pointer is lagging
       * behind the actual last element. Try to advance it one node. */
      else {
	__sync_bool_compare_and_swap(&tail, last, next);
      }
    }
  }
}

template<class T> bool LockFreeQueue<T>::nowait_deq(T &t)
{
  node *first, *last, *next;
  while (true) {
    first = head;
    last = tail;
    next = head->next; // MODIFICATION. Used to be first->next. See below.
    if (first == head) {
      if (first == last) {
	if (next == NULL)
	  return false;
	/* The tail pointer is lagging behind the actual last element.
	 * Try to advance it one node. */
	else 
	  __sync_bool_compare_and_swap(&tail, last, next);
      }
      else {
	t = next->t;
	/* If this succeeds, we've successfully swung the head pointer to
	 * next and extracted its value. */
	if (__sync_bool_compare_and_swap(&head, first, next)) {
	  /* ADDITION. Should never sleep because if we have found an item,
	   * the value of the semaphore should be at least 1. */
	  sz.down();
	  /* ADDITION. IF I HAVE INTRODUCED A BUG, THIS IS PROBABLY WHERE
	   * IT IS. We have to show that no one else could be dereferencing
	   * first (i.e. the old head) when we delete it. enq only reads
	   * from the tail, therefore the only way it could read the old
	   * head would be if the old head equaled the tail. In that case,
	   * the test first == last above would have been true, so we could
	   * not be here. So the only other case we have to worry about is
	   * another thread executing wait_deq or nowait_deq (which are 
	   * basically the same functions). Suppose another thread
	   * executes first = head above, then we do our CAS to swing
	   * the head pointer ahead. Then when the other thread does
	   * the test first == head, it will be false, so the thread
	   * will safely loop.
	   *
	   * We replaced the assignment next = first->next with
	   * the assignment next = head->next to avoid dereferencing
	   * a freed pointer. The only time these assignments are different
	   * is when the head pointer gets swung ahead between the assignment
	   * first = head and the assignment next = head->next. In this case,
	   * again, the test first == head will be false, and we will loop
	   * harmlessly. */
	  delete first;
	  return true;
	}
      }
    }
  }
}

// All the comments to nowait_deq apply here.
template<class T> T LockFreeQueue<T>::wait_deq()
{
  node *first, *last, *next;
  T ans;
  while (true) {
    first = head;
    last = tail;
    next = head->next; // MODIFICATION. Used to be first->next.
    if (first == head) {
      if (first == last) {
	if (next == NULL) {
	  sz.down(); // ADDITION. Will go to sleep until an enq.
	  sz.up(); // ADDITION. We do this because we didn't dequeue anything.
	}
	else 
	  __sync_bool_compare_and_swap(&tail, last, next);
      }
      else {
	ans = next->t;
	if (__sync_bool_compare_and_swap(&head, first, next)) {
	  sz.down(); // ADDITION
	  delete first; // ADDITION
	  return ans;
	}
      }
    }
  }
}

#endif // LOCK_FREE_QUEUE_HPP
