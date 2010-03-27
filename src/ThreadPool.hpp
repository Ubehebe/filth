#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <algorithm>
#include <list>
#include <signal.h>
#include <string.h>

#include "Factory.hpp"
#include "Locks.hpp"
#include "Thread.hpp"

template<class T> class ThreadPool
{
  Mutex m;
  CondVar done;
  std::list<Thread<T> *> threads;
  static void cleanup(void *thethreadpool);
  static void pthread_exit_wrapper(int ignore) { _LOG_DEBUG("got %s", strsignal(ignore)); pthread_exit(NULL); }
  int sigkill;
public:
  ThreadPool(Factory<T> &f, void (T::*todo)(), int nthreads, int sigkill=-1);
  ~ThreadPool();
  static void UNSAFE_emerg_yank(int ignore=-1);
  void start();

  static ThreadPool<T> *thethreadpool;
};

template<class T> ThreadPool<T> *ThreadPool<T>::thethreadpool;

template<class T> ThreadPool<T>::ThreadPool(Factory<T> &f,
					    void (T::*todo)(), int nthreads, int sigkill)
  : done(m), sigkill(sigkill)
{
  thethreadpool = this;

  /* Either block all signals to the threads in the pool, or block all
   * signals except the one the pool uses internally to signal them. */
  sigset_t toblock;
  if (sigfillset(&toblock)==-1) {
    _LOG_FATAL("sigfillset: %m");
    exit(1);
  }
  if (sigkill != -1) {
    if (sigdelset(&toblock, sigkill)==-1) {
      _LOG_FATAL("sigdelset: %m");
      exit(1);
    }
    struct sigaction act;
    memset((void *)&act, 0, sizeof(act));
    act.sa_handler = pthread_exit_wrapper;
    if (sigfillset(&act.sa_mask)==-1) {
      _LOG_FATAL("sigfillset: %m");
      exit(1);
    }
    if (sigaction(sigkill, &act, NULL)==-1) {
      _LOG_FATAL("sigaction: %m");
      exit(1);
    }
    _LOG_INFO("Redefined the disposition of signal \"%s\". "
	      "This affects every thread in the process, "
	      "including ones we know nothing about. Beware.",
	      strsignal(sigkill));
  }

  m.lock();
  for (int i=0; i<nthreads; ++i)
    threads.push_back(new Thread<T>(f, todo, &toblock, true, cleanup,
				    reinterpret_cast<void *>(this)));
  m.unlock();
}

/* There's a suggestive bug going on here: although every thread
 * in the pool receives the signal, only ONE thread executes this function.
 * The result is deadlock because the ThreadPool destructor is waiting for
 * the thread count to dwindle to zero. Why does this happen? */
template<class T> void ThreadPool<T>::cleanup(void *tpool)
{
  ThreadPool<T> *tp = reinterpret_cast<ThreadPool<T> *>(tpool);
  std::list<Thread<T> *> &threads = tp->threads;
  typename std::list<Thread<T> *>::iterator it;
  pthread_t me = pthread_self();
  tp->m.lock();
  _LOG_DEBUG("%d threads", threads.size());
  for (it = threads.begin(); it != threads.end(); ++it) {
    if (pthread_equal(me, (*it)->th)) {
      _LOG_DEBUG("found myself");
      delete *it; // By this time we shouldn't be using any of this
      threads.erase(it);
      if (threads.empty()) {
	tp->done.signal();
      }
      break;
    }
  }
  if (it == threads.end())
    _LOG_INFO("exiting thread didn't find itself in thread pool");
  
  tp->m.unlock();
}

template<class T> ThreadPool<T>::~ThreadPool()
{
  m.lock();
  while (!threads.empty()) {
    _LOG_DEBUG("ThreadPool destructor waiting: %d left", threads.size());
    done.wait();
  }
  m.unlock();
  _LOG_DEBUG("ThreadPool destructor complete");
}

template<class T> void ThreadPool<T>::start()
{
  /* This locking is needed because when the threads finish, they try to
   * remove themselves from this very same list. If we didn't lock and a thread
   * finished before this function was done, chaos could ensue. */
  m.lock();
  std::for_each(threads.begin(), threads.end(), std::mem_fun(&Thread<T>::start));
  m.unlock();
}

template<class T> void ThreadPool<T>::UNSAFE_emerg_yank(int ignore)
{
  std::list<Thread<T> *> &threads = thethreadpool->threads;
  typename std::list<Thread<T> *>::iterator it;
  if (thethreadpool->sigkill != -1) {
    thethreadpool->m.lock();
    for (it = threads.begin(); it != threads.end(); ++it) {
      _LOG_DEBUG("kill sent");
      if ((errno = pthread_kill((*it)->th, thethreadpool->sigkill))!=0)
	_LOG_INFO("pthread_kill: %m, continuing");
      //      delete *it; // Is this safe?
    }
    //    threads.clear();
    //    thethreadpool->done.signal(); // ???
    thethreadpool->m.unlock();
    //    thethreadpool = NULL; // ???
  }
}

#endif // THREAD_POOL_HPP
