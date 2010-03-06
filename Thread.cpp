#ifndef THREAD_CPP
#define THREAD_CPP

#include <errno.h>
#include <functional>
#include <stdio.h>
#include <unistd.h>

template<class C> sigset_t Thread<C>::default_sigmask;
template<class C> bool Thread<C>::default_sigmask_set = false;

template<class C> void Thread<C>::set_default_sigmask(sigmasks::builtin d)
{

  switch (d) {
  case sigmasks::BLOCK_NONE:
  if (sigemptyset(&default_sigmask)==-1) {
    perror("sigemptyset");
    abort();
  }
  break;
  case sigmasks::BLOCK_ALL:
    if (sigfillset(&default_sigmask)==-1) {
      perror("sigfillset");
      abort();
    }
    break;
  }
  default_sigmask_set = true;
}

template<class C> void Thread<C>::set_default_sigmask(std::vector<int> sigmask)
{
  if (sigemptyset(&default_sigmask)==-1) {
    perror("sigemptyset");
    abort();
  }

  for (std::vector<int>::iterator it = sigmask.begin(); it != sigmask.end(); ++it) {
    if (sigaddset(&default_sigmask, *it)==-1) {
      perror("sigaddset");
      abort();
    }
  }
  default_sigmask_set = true;
}

template<class C> Thread<C>::Thread(void (C::*p)())
  : _c(new C()), _p(p), dodelete(true), _sigmask(default_sigmask)
{
  if (!default_sigmask_set)
    set_default_sigmask(sigmasks::BLOCK_NONE);
  _init();
}

template<class C> Thread<C>::Thread(C *c, void (C::*p)())
  : _c(c), _p(p), dodelete(false), _sigmask(default_sigmask)
{
  if (!default_sigmask_set)
    set_default_sigmask(sigmasks::BLOCK_NONE);
  _init();
}

template<class C> Thread<C>::Thread(void (C::*p)(), std::vector<int> sigmask)
  : _c(new C()), _p(p), dodelete(true)
{
  set_sigmask(sigmask);
  _init();
}

template<class C> Thread<C>::Thread(void (C::*p)(), sigmasks::builtin d)
  : _c(new C()), _p(p), dodelete(true)
{
  set_sigmask(d);
  _init();
}

template<class C> Thread<C>::Thread(C *c, void (C::*p)(), std::vector<int> sigmask)
  : _c(c), _p(p), dodelete(false)
{
  set_sigmask(sigmask);
  _init();
}

template<class C> Thread<C>::Thread(C *c, void (C::*p)(), sigmasks::builtin d)
  : _c(c), _p(p), dodelete(false)
{
  set_sigmask(d);
  _init();
}

template<class C> void Thread<C>::set_sigmask(std::vector<int> mask)
{
  if (sigemptyset(&_sigmask)==-1) {
    perror("sigemptyset");
    abort();
  }
  for (std::vector<int>::iterator it = mask.begin(); it != mask.end(); ++it) {
    if (sigaddset(&_sigmask, *it)==-1) {
      perror("sigaddset");
      abort();
    }
  }
}

template<class C> void Thread<C>::set_sigmask(sigmasks::builtin d)
{
  switch (d) {
  case sigmasks::BLOCK_NONE:
    if (sigemptyset(&_sigmask)==-1) {
      perror("sigemptyset");
      abort();
    }
    break;
  case sigmasks::BLOCK_ALL:
    if (sigfillset(&_sigmask)==-1) {
      perror("sigfillset");
      abort();
    }
    break;
  }
}

template<class C> void Thread<C>::_init()
{
  _Thread *tmp = new _Thread(_c, _p, &_sigmask);
  if ((errno = pthread_create(&th, NULL, Thread<C>::pthread_create_wrapper,
			      reinterpret_cast<void *>(tmp))) != 0) {
    perror("pthread_create");
    abort();
  }
}

template<class C> void *Thread<C>::pthread_create_wrapper
(void *_Th)
{
  _Thread *tmp = reinterpret_cast<_Thread *>(_Th);

  if (pthread_sigmask(SIG_SETMASK, tmp->_sigmask, NULL) != 0) {
    perror("pthread_sigmask");
    abort();
  }

  (std::mem_fun(tmp->_p))(tmp->_c);
  delete tmp;
}

template<class C> void Thread<C>::join()
{
  if (pthread_join(th, NULL)!=0) {
    perror("pthread_join");
    abort();
  }
}

template<class C> Thread<C>::~Thread() 
{
  join(); 
  if (dodelete) 
    delete _c;
}

#endif // THREAD_CPP
