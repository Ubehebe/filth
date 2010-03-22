#ifndef THREAD_HPP
#define THREAD_HPP

#include <algorithm>
#include <errno.h>
#include <functional>
#include <list>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
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

  static int sigth;
  static void setup_emerg_exitall(int sigtouse);
  static void sigall(int ignore);

private:
  C *_c;
  void (C::*_p)();
  bool dodelete;
  pthread_t th;
  int cancelstate, canceltype; // pthreads cancel

  static std::list<pthread_t> ths;
  static void emerg_exit(int ignore) { pthread_exit(NULL); }
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
	_canceltype(_canceltype)
    {}
  };
};

template<class C> int Thread<C>::sigth= -1;

template<class C> std::list<pthread_t> Thread<C>::ths;

template<class C> void Thread<C>::setup_emerg_exitall(int sigtouse)
{
  if (sigth != -1) {
    _LOG_INFO("redefining internal-use pthread_kill signal from %d to %d; "
	      "expect chaos", sigth, sigtouse);
  }
  sigth = sigtouse;

  struct sigaction act;
  memset((void *)&act, 0, sizeof(act));
  act.sa_handler = emerg_exit;
  if (sigaction(sigth, &act, NULL)==-1) {
    _LOG_FATAL("sigaction: %m");
    exit(1);
  }
}

template<class C> void Thread<C>::sigall(int ignore)
{
  for (std::list<pthread_t>::iterator it = ths.begin(); it != ths.end(); ++it) {
    if ((errno=pthread_kill(*it, sigth))!=0)
      _LOG_INFO("pthread_kill: %m, continuing");
  }
  ths.clear();
}

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

  if (sigth != -1) {
    sigmasks::sigmask_caller(SIG_UNBLOCK, sigth);
    ths.push_back(pthread_self());
  }

  _LOG_DEBUG("%ld", syscall(SYS_gettid));

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
  _LOG_DEBUG("blocking at pthread_join");
  if ((errno = pthread_join(th, NULL))!=0) {
    _LOG_FATAL("pthread_join: %m");
    exit(1);
  }
  _LOG_DEBUG("finished pthread_join");
  if (dodelete) 
    delete _c;
}

template<class C> void Thread<C>::cancel()
{
  if ((errno = pthread_cancel(th))!=0)
    _LOG_INFO("pthread_cancel: %m, continuing");
}

#endif // THREAD_HPP
