#ifndef THREAD_HPP
#define THREAD_HPP

#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <functional>
#include <stdlib.h>
#include <unistd.h>

#include "logging.h"
#include "sigmasks.hpp"

/* Wrapper around posix threads to do the most common thing:
 * set up a thread to execute a nullary member function.
 * 
 * Notes:
 *
 * The destructor performs the pthread_wait; I think this is reasonable,
 * but it means you have to be careful about when your Thread objects
 * go out of scope.
 *
 * Two of the constructors take a sigmasks::builtin argument, which
 * receive the interpretation of "block these"; e.g. the constructor
 * interprets sigmasks::ALL to mean "block all signals". If a constructor
 * is called without a sigmasks::builtin argument, we assume the
 * object is going to set its own signal mask, perhaps as the first thing
 * in the nullary function. We didn't provide more flexibility in the
 * constructor on purpose; if a thread is going to have an elaborate
 * signal mask, it is natural that the object that becomes the thread
 * should know how to set it. */
template<class C> class Thread
{
  // No copying, no assigning.
  Thread(Thread const&);
  Thread &operator=(Thread const&);

public:
  Thread(void (C::*p)());
  Thread(void (C::*p)(), sigmasks::builtin b);
  Thread(C *c, void (C::*p)());
  Thread(C *c, void (C::*p)(), sigmasks::builtin b);
  ~Thread();
private:
  C *_c;
  void (C::*_p)();

  bool dodelete;
  pthread_t th;

  static void *pthread_create_wrapper(void *_Th);

  void _init(sigmasks::builtin *b);
  // Just to pack and unpack stuff across the call to pthread_create_wrapper.
  struct _Thread
  {
    C *_c;
    void (C::*_p)();
    sigmasks::builtin *_b;
    _Thread(C *_c, void (C::*_p)(), sigmasks::builtin *_b)
      : _c(_c), _p(_p), _b(_b) {}
  };
};



template<class C> Thread<C>::Thread(void (C::*p)())
  : _c(new C()), _p(p), dodelete(true)
{
  _init(NULL);
}

template<class C> Thread<C>::Thread(C *c, void (C::*p)())
  : _c(c), _p(p), dodelete(false)
{
  _init(NULL);
}

template<class C> Thread<C>::Thread(void (C::*p)(), sigmasks::builtin b)
  : _c(new C()), _p(p), dodelete(true)
{
  _init(&b);
}

template<class C> Thread<C>::Thread(C *c, void (C::*p)(), sigmasks::builtin b)
  : _c(c), _p(p), dodelete(false)
{
  _init(&b);
}

template<class C> void Thread<C>::_init(sigmasks::builtin *b)
{
  _Thread *tmp = new _Thread(_c, _p, b);
  if ((errno = pthread_create(&th, NULL, Thread<C>::pthread_create_wrapper,
			      reinterpret_cast<void *>(tmp))) != 0) {
    _LOG_FATAL("pthread_create: %m");
    exit(1);
  }
}

template<class C> void *Thread<C>::pthread_create_wrapper
(void *_Th)
{
  _Thread *tmp = reinterpret_cast<_Thread *>(_Th);

  /* This thread inherits the signal mask of the process
   * that called pthread_create. Thus, there could be a race
   * condition if the caller doesn't block signal X, this thread
   * does block signal X, but this thread receives signal X
   * before setting its sigmask. Equally, if the caller blocks signal X,
   * this thread doesn't, but signal X is generated before setting its
   * sigmask, the signal could be lost. */
  if (tmp->_b != NULL)
    sigmasks::sigmask_caller(*(tmp->_b));
  
  (std::mem_fun(tmp->_p))(tmp->_c);
  delete tmp;
}

template<class C> Thread<C>::~Thread() 
{
  pthread_join(th, NULL);
  /*  if (pthread_join(th, NULL)!=0) {
    _LOG_FATAL("pthread_join: %m");
    exit(1);
    }*/
  if (dodelete) 
    delete _c;
}

#endif // THREAD_HPP
