#ifndef SIGTHREAD_HPP
#define SIGTHREAD_HPP

#include <list>
#include <signal.h>
#include <string.h>

#include "Callback.hpp"
#include "logging.h"
#include "Thread.hpp"

/* There is something seriously wrong with the rendezvous
 * mechanism in this class. Do not use until further notice. */

struct registerth : public Callback
{
  std::list<pthread_t> ths;
  Mutex m;
  CondVar c;
  registerth() : c(m) {}
  void operator()();
  void wait();
};

template<class C> class SigThread : public Thread<C>
{
  static void thexit(int ignore)
  {
    _LOG_DEBUG("exiting asynchronously");
    cleanup(NULL);
    pthread_exit(NULL);
  }
public:
  SigThread(void (C::*p)())
    : Thread<C>(p, &sigmask, true, cleanup) {}
  SigThread(C *c, void (C::*p)())
    : Thread<C>(c, p, &sigmask, true, cleanup) {}
  static void setup(int sigexternal, int siginternal, void (*handler)(int)=thexit);
  static void setup(int siginternal, void (*handler)(int)=thexit);
  static void setup();
  static void wait() { thcb.wait(); }
  static void sigall(int ignore);
private:
  static sigset_t sigmask;
  static int siginternal;
  static void cleanup(void *ignore);
  static registerth thcb;
};

template<class C> int SigThread<C>::siginternal;
template<class C> sigset_t SigThread<C>::sigmask;
template<class C> registerth SigThread<C>::thcb;

void registerth::operator()()
{
  m.lock();
  ths.push_back(pthread_self());
  m.unlock();
}

template<class C>
void SigThread<C>::setup(int sigexternal, int siginternal, void (*handler)(int))
{
  struct sigaction act;
  memset((void *)&act, 0, sizeof(act));
  act.sa_handler = sigall;
  // Block all signals during the execution of the handler.
  if (sigfillset(&act.sa_mask)==-1) {
    _LOG_FATAL("sigfillset: %m");
    exit(1);
  }
  if (sigaction(sigexternal, &act, NULL)==-1) {
    _LOG_FATAL("sigaction: %m");
    exit(1);
  }
  setup(siginternal, handler);
}

template<class C>
void SigThread<C>::setup(int siginternal, void (*handler)(int))
{
  SigThread<C>::siginternal = siginternal;
  struct sigaction act;
  memset((void *)&act, 0, sizeof(act));
  act.sa_handler = handler;
  // Block all signals during the execution of the handler.
  if (sigfillset(&act.sa_mask)==-1) {
    _LOG_FATAL("sigfillset: %m");
    exit(1);
  }
  if (sigaction(siginternal, &act, NULL)==-1) {
    _LOG_FATAL("sigaction: %m");
    exit(1);
  }
  // Now set the static signal mask that all SigThread<C>'s will use.
  if (sigfillset(&sigmask)==-1) {
    _LOG_FATAL("sigfillset: %m");
    exit(1);
  }
  if (sigdelset(&sigmask, siginternal)==-1) {
    _LOG_FATAL("sigdelset: %m");
    exit(1);
  }
  setup();
}

template<class C> void SigThread<C>::setup()
{
  Thread<C>::thcb = &thcb;
}

void registerth::wait()
{
  m.lock();
  while (!ths.empty())
    c.wait();
  m.unlock();
}

template<class C> void SigThread<C>::sigall(int ignore)
{
  thcb.m.lock();
  _LOG_DEBUG("siginternal %s", strsignal(siginternal));
  for (std::list<pthread_t>::iterator it = thcb.ths.begin();
       it != thcb.ths.end(); ++it) {
    if ((errno = pthread_kill(*it, siginternal))!=0)
      _LOG_INFO("pthread_kill: %m, continuing");
  }
  thcb.m.unlock();
}

// TODO: a mini-treatise on how really to kill threads asynchronously =)
template<class C> void SigThread<C>::cleanup(void *ignore)
{
  std::list<pthread_t>::iterator it;
  pthread_t me = pthread_self();
  thcb.m.lock();
  for (it = thcb.ths.begin(); it != thcb.ths.end(); ++it) {
    if (pthread_equal(me, *it))
      break;
  }
  if (it != thcb.ths.end()) {
    thcb.ths.erase(it);
    if (thcb.ths.empty())
      thcb.c.signal();
  }
  else {
    _LOG_FATAL("inconsistency in thread cleanup");
    exit(1);
  }
  thcb.m.unlock();
}

#endif // ASYNC_KILLABLE_THREAD_HPP
