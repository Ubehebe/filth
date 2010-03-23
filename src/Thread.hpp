#ifndef THREAD_HPP
#define THREAD_HPP

#include <errno.h>
#include <functional>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "Locks.hpp"
#include "logging.h"

// TODO: add push at exit handlers
// Direction: condition variables to signal deallocation...works better than join?

/* Wrapper around posix threads to do the most common thing:
 * set up a thread to execute a nullary member function.
 * 
 * Notes:
 *
 * UPDATE NOTES! */
template<class C> class Thread
{
  static void cleanup_default(void *) {}
public:
  Thread(void (C::*p)(),
	 sigset_t *_sigmask=NULL,
	 bool detached=false,
	 void (*cleanup)(void *)=cleanup_default,
	 bool cleanup_on_normal_exit=true,
	 int cancelstate=PTHREAD_CANCEL_ENABLE,
	 int canceltype=PTHREAD_CANCEL_DEFERRED);
  Thread(C *c,
	 void (C::*p)(),
	 sigset_t *_sigmask=NULL,
	 bool detached=false,
	 void (*cleanup)(void *)=cleanup_default,
	 bool cleanup_on_normal_exit=true,
	 int cancelstate=PTHREAD_CANCEL_ENABLE,
	 int canceltype=PTHREAD_CANCEL_DEFERRED);
  ~Thread();
  void cancel(); // Found no use for this yet

protected:
  pthread_t th;
  sigset_t *_sigmask;

private:
  Thread(Thread const&);
  Thread &operator=(Thread const&);
  C *_c;
  void (C::*_p)();
  bool dodelete, detached, cleanup_on_normal_exit;
  int cancelstate, canceltype;
  void (*cleanup)(void *);

  static void *pthread_create_wrapper(void *_Th);

  void _init();

  // Just to pack and unpack stuff across the call to pthread_create_wrapper.
  struct _Thread
  {
    C *_c;
    void (C::*_p)();
    sigset_t *_sigmask;
    int _cancelstate, _canceltype, _cleanup_on_normal_exit;
    void (*_cleanup)(void *);
    _Thread(C *_c, void (C::*_p)(), sigset_t *_sigmask,
	    void (*_cleanup)(void *), bool _cleanup_on_normal_exit,
	    int _cancelstate, int _canceltype)
      : _c(_c), _p(_p), _sigmask(_sigmask),  _cleanup(_cleanup),
	_cleanup_on_normal_exit(static_cast<int>(_cleanup_on_normal_exit)),
	_cancelstate(_cancelstate), _canceltype(_canceltype)
    {}
  };
};

template<class C> Thread<C>::Thread(
				    void (C::*p)(),
				    sigset_t *_sigmask,
				    bool detached,
				    void (*cleanup)(void *),
				    bool cleanup_on_normal_exit,
				    int cancelstate,
				    int canceltype)
  : _c(new C()), _p(p), _sigmask(_sigmask), detached(detached),
    cleanup(cleanup), cleanup_on_normal_exit(cleanup_on_normal_exit),
    cancelstate(cancelstate), canceltype(canceltype), dodelete(true)
{
  _init();
}

template<class C> Thread<C>::Thread(
				    C *c,
				    void (C::*p)(),
				    sigset_t *_sigmask,
				    bool detached,
				    void (*cleanup)(void *),
				    bool cleanup_on_normal_exit,
				    int cancelstate,
				    int canceltype)
  : _c(c), _p(p), _sigmask(_sigmask), detached(detached), cleanup(cleanup),
    cleanup_on_normal_exit(cleanup_on_normal_exit),
    cancelstate(cancelstate), canceltype(canceltype), dodelete(false)
{
  _init();
}

template<class C> void Thread<C>::_init()
{
  _Thread *tmp = new _Thread(_c, _p, _sigmask, cleanup,
			     cleanup_on_normal_exit, cancelstate, canceltype);

  pthread_attr_t attr;
  if ((errno = pthread_attr_init(&attr))!=0) {
    _LOG_FATAL("pthread_attr_init: %m");
    exit(1);
  }
  if ((errno = pthread_attr_setdetachstate(&attr,
					   (detached)
					   ? PTHREAD_CREATE_DETACHED
					   : PTHREAD_CREATE_JOINABLE))!=0) {
    _LOG_FATAL("pthread_attr_setdetachstate: %m");
    exit(1);
  }

  if ((errno = pthread_create(&th, &attr, Thread<C>::pthread_create_wrapper,
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
  if (tmp->_sigmask != NULL
      && ((errno = pthread_sigmask(SIG_SETMASK, tmp->_sigmask, NULL))!=0)) {
    _LOG_FATAL("pthread_sigmask: %m");
    exit(1);
  }
  if ((errno = pthread_setcancelstate(tmp->_cancelstate, NULL))!=0) {
    _LOG_FATAL("pthread_setcancelstate: %m");
    exit(1);
  }
  if ((errno = pthread_setcanceltype(tmp->_canceltype, NULL))!=0) {
    _LOG_FATAL("pthread_setcanceltype: %m");
    exit(1);
  }

  void (*cleanup)(void *) = tmp->_cleanup;
  bool cleanup_on_normal_exit = tmp->_cleanup_on_normal_exit;
  void (C::*p)() = tmp->_p;
  C *c = tmp->_c;
  
  delete tmp;

  pthread_cleanup_push(cleanup, NULL);

  (std::mem_fun(p))(c);

  pthread_cleanup_pop(cleanup_on_normal_exit);

}

template<class C> Thread<C>::~Thread() 
{
  if (!detached && (errno = pthread_join(th, NULL))!=0) {
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
