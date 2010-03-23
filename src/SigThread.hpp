#ifndef SIGTHREAD_HPP
#define SIGTHREAD_HPP

#include <list>
#include <signal.h>
#include <string.h>

#include "logging.h"
#include "Thread.hpp"

template<class C> class SigThread : public Thread<C>
{
  static void thexit(int ignore)
  {
    _LOG_DEBUG("exiting asynchronously");
    cleanup(NULL);
    pthread_exit(NULL);
  }
public:
  SigThread(void (C::*p)());
  SigThread(C *c, void (C::*p)());
  static void setup(int sigexternal, int siginternal, void (*handler)(int)=thexit);
  static void setup(int siginternal, void (*handler)(int)=thexit);
  static void wait();
  static void sigall(int ignore);
private:
  static sigset_t sigmask;
  static std::list<pthread_t> ths;
  static void cleanup(void *ignore);
  static int siginternal;
  static Mutex m;
  static CondVar c;
};

template<class C> std::list<pthread_t> SigThread<C>::ths;
template<class C> int SigThread<C>::siginternal;
template<class C> Mutex SigThread<C>::m;
template<class C> CondVar SigThread<C>::c(m); // Could cause linker errors
template<class C> sigset_t SigThread<C>::sigmask;

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
}

template<class C> SigThread<C>::SigThread(void (C::*p)())
  : Thread<C>(p, &sigmask, true, cleanup)
{
  m.lock();
  ths.push_back(this->th);
  m.unlock();
}

template<class C> SigThread<C>::SigThread(C *c, void (C::*p)())
  : Thread<C>(c, p, &sigmask, true, cleanup)
{
  m.lock();
  ths.push_back(this->th);
  m.unlock();
}

template<class C> void SigThread<C>::wait()
{
  m.lock();
  while (!ths.empty())
    c.wait();
  m.unlock();
  _LOG_DEBUG("cleared");
}

template<class C> void SigThread<C>::sigall(int ignore)
{
  m.lock();
  _LOG_DEBUG("siginternal %s", strsignal(siginternal));
  for (std::list<pthread_t>::iterator it = ths.begin();
       it != ths.end(); ++it) {
    if ((errno = pthread_kill(*it, siginternal))!=0)
      _LOG_INFO("pthread_kill: %m, continuing");
  }
  m.unlock();
}

// TODO: a mini-treatise on how really to kill threads asynchronously =)
template<class C> void SigThread<C>::cleanup(void *ignore)
{
  _LOG_DEBUG("cleanup on exit");
  std::list<pthread_t>::iterator it;
  pthread_t me = pthread_self();
  m.lock();
  for (it = ths.begin(); it != ths.end(); ++it) {
    if (pthread_equal(me, *it))
      break;
  }
  if (it != ths.end()) {
    ths.erase(it);
    if (ths.empty())
      c.signal();
  }
  m.unlock();
}

#endif // ASYNC_KILLABLE_THREAD_HPP
