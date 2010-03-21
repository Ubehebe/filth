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
  Thread(Thread const&);
  Thread &operator=(Thread const&);

public:
  Thread(void (C::*p)(),
	 int cancelstate=PTHREAD_CANCEL_ENABLE,
	 int canceltype=PTHREAD_CANCEL_DEFERRED);
  Thread(void (C::*p)(), sigmasks::builtin b,
	 int cancelstate=PTHREAD_CANCEL_ENABLE,
	 int canceltype=PTHREAD_CANCEL_DEFERRED);
  Thread(C *c, void (C::*p)(),
	 int cancelstate=PTHREAD_CANCEL_ENABLE,
	 int canceltype=PTHREAD_CANCEL_DEFERRED);
  Thread(C *c, void (C::*p)(), sigmasks::builtin b,
	 int cancelstate=PTHREAD_CANCEL_ENABLE,
	 int canceltype=PTHREAD_CANCEL_DEFERRED);
  ~Thread();
  void cancel();
private:
  C *_c;
  void (C::*_p)();

  bool dodelete;
  pthread_t th;
  int cancelstate, canceltype;

  static void *pthread_create_wrapper(void *_Th);

  void _init(sigmasks::builtin *b);
  // Just to pack and unpack stuff across the call to pthread_create_wrapper.
  struct _Thread
  {
    C *_c;
    void (C::*_p)();
    sigmasks::builtin *_b;
    int _cancelstate, _canceltype;
    _Thread(C *_c, void (C::*_p)(), sigmasks::builtin *_b,
	    int _cancelstate, int _canceltype)
      : _c(_c), _p(_p), _b(_b),  _cancelstate(_cancelstate),
	_canceltype(_canceltype) {}
  };
};



template<class C> Thread<C>::Thread(void (C::*p)(),
				    int cancelstate, int canceltype)
  : _c(new C()), _p(p), dodelete(true),
    cancelstate(cancelstate), canceltype(canceltype)
{
  _init(NULL);
}

template<class C> Thread<C>::Thread(C *c, void (C::*p)(),
				    int cancelstate, int canceltype)
  : _c(c), _p(p), dodelete(false),
    cancelstate(cancelstate), canceltype(canceltype)
{
  _init(NULL);
}

template<class C> Thread<C>::Thread(void (C::*p)(), sigmasks::builtin b,
				    int cancelstate, int canceltype)
  : _c(new C()), _p(p), dodelete(true),
    cancelstate(cancelstate), canceltype(canceltype)
{
  _init(&b);
}

template<class C> Thread<C>::Thread(C *c, void (C::*p)(), sigmasks::builtin b,
				    int cancelstate, int canceltype)
  : _c(c), _p(p), dodelete(false),
    cancelstate(cancelstate), canceltype(canceltype)
{
  _init(&b);
}

template<class C> void Thread<C>::_init(sigmasks::builtin *b)
{
  _Thread *tmp = new _Thread(_c, _p, b, cancelstate, canceltype);
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

  if ((errno = pthread_setcancelstate(tmp->_cancelstate, NULL))!=0) {
    _LOG_FATAL("pthread_setcancelstate: %m");
    exit(1);
  }
  if ((errno = pthread_setcanceltype(tmp->_canceltype, NULL))!=0) {
    _LOG_FATAL("pthread_setcanceltype: %m");
    exit(1);
  }
  
  (std::mem_fun(tmp->_p))(tmp->_c);
  delete tmp;
}

template<class C> Thread<C>::~Thread() 
{
  if ((errno = pthread_join(th, NULL))!=0) {
    _LOG_FATAL("pthread_join: %m");
    exit(1);
  }
  if (dodelete) 
    delete _c;
}

template<class C> void Thread<C>::cancel()
{
  if ((errno = pthread_cancel(th))!=0)
    _LOG_INFO("pthread_cancel: %m, continuing");
}

#endif // THREAD_HPP
