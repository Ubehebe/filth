#ifndef PREALLOCATED_HPP
#define PREALLOCATED_HPP

#include <stdint.h>

#include "LockFreeQueue.hpp"

/* A base class allowing objects to do their own thread-safe allocation.
 * N.B. I'm using the lock-free queue now, but it's actually not correct; 
 * it suffers from the ABA problem. The ABA problem occurs only under
 * extraordinary scheduling conditions, e.g. the OS suspends a thread
 * for a really long time. But if you see crashes, feel free to sub in the
 * double-locked queue, which is correct; they have the same interface.
 *
 * The odd template pattern (e.g., class Foo : public Preallocated<Foo>)
 * is just a way of requiring that all instances of Foo share a static store.
 * If the store was static without the class being a template, all classes
 * inheriting from Preallocated would share the exact same static store,
 * surely not what we want. */
template<class Derived> class Preallocated
{
public:
  void *operator new(size_t sz);
  void operator delete(void *stuff);
  static void prealloc_init(size_t prealloc_chunks);
  static void prealloc_deinit();
private:
  static LockFreeQueue<void *> _store;
};

template<class Derived> LockFreeQueue<void *> Preallocated<Derived>::_store;

template<class Derived>
void *Preallocated<Derived>::operator new(size_t sz)
{
  void *stuff;
  if (!_store.nowait_deq(stuff)) {
    stuff = ::operator new(sz);
  }
  return stuff;
}

template<class Derived>
void Preallocated<Derived>::operator delete(void *stuff)
{
  _store.enq(stuff);
}

template<class Derived>
void Preallocated<Derived>::prealloc_init(size_t prealloc_chunks)
{
  /* This is not great, since if the queue is e.g. coarse locked, we acquire
   * and release the lock prealloc_chunks times. however it's just startup
   * code. */
  for (int i=0; i<prealloc_chunks; ++i)
    _store.enq(reinterpret_cast<void *>(new uint8_t[sizeof(Derived)]));
}

template<class Derived>
void Preallocated<Derived>::prealloc_deinit()
{
  void *tmp;
  while (_store.nowait_deq(tmp))
    delete[] reinterpret_cast<uint8_t *>(tmp);
}


#endif // PREALLOCATED_HPP
