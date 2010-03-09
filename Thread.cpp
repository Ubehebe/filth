#ifndef THREAD_CPP
#define THREAD_CPP

#include <errno.h>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    perror("pthread_create");
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
  if (pthread_join(th, NULL)!=0) {
    perror("pthread_join");
    exit(1);
  }
  if (dodelete) 
    delete _c;
}

#endif // THREAD_CPP
