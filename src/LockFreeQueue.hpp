#ifndef LOCK_FREE_QUEUE_HPP
#define LOCK_FREE_QUEUE_HPP

#include <stdint.h>
#include <iostream>

#include "ConcurrentQueue.hpp"

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
  Semaphore sz;
public:
  LockFreeQueue() : head(new node()), tail(head) {}
  void enq(T t);
  T wait_deq();
  bool nowait_deq(T &t);
};

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
    next = first->next;
    if (first == head) {
      if (first == last) {
	// Probably empty.
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
	  sz.down();
	  return true;
	}
      }
    }
  }
}

template<class T> T LockFreeQueue<T>::wait_deq()
{
  node *first, *last, *next;
  T ans;
  while (true) {
    first = head;
    last = tail;
    next = first->next;
    if (first == head) {
      if (first == last) {
	// Probably empty.
	if (next == NULL) {
	  sz.down(); // Should sleep.
	  sz.up();
	}
	/* The tail pointer is lagging behind the actual last element.
	 * Try to advance it one node. */
	else 
	  __sync_bool_compare_and_swap(&tail, last, next);
      }
      else {
	ans = next->t;
	/* If this succeeds, we've successfully swung the head pointer to
	 * next and extracted its value. */
	if (__sync_bool_compare_and_swap(&head, first, next)) {
	  sz.down();
	  return ans;
	}
      }
    }
  }
}

#endif // LOCK_FREE_QUEUE_HPP
