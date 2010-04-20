#ifndef PREALLOCATED_HPP
#define PREALLOCATED_HPP

#include <stdint.h>

#include "LockFreeQueue.hpp"

/** \brief A base class for objects doing their own thread-safe allocation.
 * \warning I'm using the LockFreeQueue now, but it's actually not correct; 
 * it suffers from the ABA problem. The ABA problem occurs only under
 * extraordinary scheduling conditions, e.g. the OS suspends a thread
 * for a really long time. But if you see crashes, feel free to sub in the
 * DoubleLockedQueue, which is correct; they have the same interface.
 * \note The odd template pattern (e.g., class Foo : public Preallocated<Foo>)
 * is just a way of requiring that all instances of Foo share a static store.
 * If the store was static without the class being a template, all classes
 * inheriting from Preallocated would share the exact same static store,
 * surely not what we want. */
template<class Derived> class Preallocated
{
public:
  /** \brief Get a new object from the internal store; if that fails, try to get
   * memory from the OS.
   * \param sz size of the Derived class. Normally not used directly;
   * \code Derived *d = new Derived(...); \endcode does the right thing. */
  void *operator new(size_t sz);
  /** \brief Return the object to the internal store.
   * \param stuff the object to be deleted. Normally not used directly;
   * \code Derived *d = new Derived(...); ... delete d; \endcode does the right
   * thing.
   * \note The object "returned" to the internal store need not have come from
   * the internal store. */
  void operator delete(void *stuff);
  /** \brief Do a big malloc all at once, basically.
   * \param numobjs number of objects of class Derived to allocate space for
   * \warning Mostly intended to be startup code. The ConcurrentQueue interface
   * exposes no "batch enqueue", so if for example the ConcurrentQueue
   * implementation is a coarse-locked queue, this will acquire and release the
   * lock lots of times.
   * \todo Who is supposed to call this? */
  static void prealloc_init(size_t numobjs);
  /** \brief Do a big free all at once, basically. */
  static void prealloc_deinit();
private:
  static LockFreeQueue<void *> _store; //!< the internal store
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
void Preallocated<Derived>::prealloc_init(size_t numobjs)
{
  /* This is not great, since if the queue is e.g. coarse locked, we acquire
   * and release the lock prealloc_chunks times. however it's just startup
   * code. */
  for (int i=0; i<numobjs; ++i)
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
