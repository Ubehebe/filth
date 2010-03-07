#ifndef THREAD_HPP
#define THREAD_HPP

#include <pthread.h>
#include <signal.h>

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

// Ugh! This is a crappy pattern to avoid template errors in the linker.
#include "Thread.cpp"

#endif // THREAD_HPP
