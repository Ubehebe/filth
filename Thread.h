#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <signal.h>
#include <vector>


namespace sigmasks
{
  enum builtin { BLOCK_NONE, BLOCK_ALL };
};

/* Wrapper around posix threads to do the most common thing:
 * set up a thread to execute a nullary member function. N.B. the destructor
 * performs the pthread_wait; I think this is reasonable. */
template<class C> class Thread
{
public:
  Thread(void (C::*p)());
  Thread(C *c, void (C::*p)());
  Thread(void (C::*p)(), std::vector<int> sigmask);
  Thread(void (C::*p)(), sigmasks::builtin b);
  Thread(C *c, void (C::*p)(), std::vector<int> sigmask);
  Thread(C *c, void (C::*p)(), sigmasks::builtin b);
  ~Thread();
  static void set_default_sigmask(std::vector<int> mask);
  static void set_default_sigmask(sigmasks::builtin b);
  static bool default_sigmask_set;
  void join();
private:
  C *_c;
  void (C::*_p)();
  sigset_t _sigmask;

  bool dodelete;
  pthread_t th;

  static sigset_t default_sigmask;
  static void *pthread_create_wrapper(void *_Th);

  void set_sigmask(std::vector<int> mask);
  void set_sigmask(sigmasks::builtin b);
  void _init();
  // Just to pack and unpack stuff across the call to pthread_create_wrapper.
  struct _Thread
  {
    C *_c;
    void (C::*_p)();
    sigset_t *_sigmask;
    _Thread(C *_c, void (C::*_p)(), sigset_t *_sigmask) 
      : _c(_c), _p(_p), _sigmask(_sigmask) {}
  };
};

// Ugh! This is a crappy pattern to avoid template errors in the linker.
#include "Thread.cpp"

#endif // THREAD_H
