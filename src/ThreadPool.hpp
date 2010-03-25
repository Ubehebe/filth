#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <algorithm>
#include <list>
#include <signal.h>

#include "Factory.hpp"
#include "Locks.hpp"
#include "Thread.hpp"

template<class T> class ThreadPool
{
  Mutex m;
  CondVar done;
  std::list<Thread<T> *> threads;
  static ThreadPool *thethreadpool;
  static void cleanup(void *ignore);
  static pthread_exit_wrapper(int ignore) { pthread_exit(NULL); }
public:
  ThreadPool(Factory<T> &f, void (T::*todo)(), int nthreads, int sigkill=-1);
  void start();
};

template<class T> ThreadPool<T> *ThreadPool<T>::thethreadpool;

template<class T> ThreadPool<T>::ThreadPool(Factory<T> &f,
					    void (T::*todo)(), int nthreads, int sigkill)
  : done(m)
{
  thethreadpool = this;

  sigset_t *toblock = NULL;
  if (sigkill != -1) {
    sigset_t toblock_;
    if (sigfillset(&toblock_)==-1) {
      _LOG_FATAL("sigfillset: %m");
      exit(1);
    }
    if (sigdelset(&toblock_, sigkill)==-1) {
      _LOG_FATAL("sigdelset: %m");
      exit(1);
    }
    toblock = &toblock_;
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
  }

  for (int i=0; i<nthreads; ++i)
    threads.push_back(new Thread<T>(f, todo, toblock, true, cleanup));
}

template<class T> void ThreadPool<T>::cleanup(void *ignore)
{
  std::list<Thread<T> *> &threads = thethreadpool->threads;
  std::list<Thread<T> *>::iterator it;
  pthread_t me = pthread_self();
  thethreadpool->m.lock();
  for (it = threads.begin(); it != threads.end(); ++it) {
    if (pthread_equal(me, (*it)->th)) {
      threads.erase(it);
      if (threads.empty())
	done.signal();
      break;
    }
  }
  if (it == threads.end()) {
    _LOG_FATAL("exiting thread didn't find itself in thread pool");
    exit(1);
  }
  thethreadpool->m.unlock();
}

template<class T> ThreadPool<T>::~ThreadPool()
{
  m.lock();
  while (!threads.empty())
    done.wait();
  m.unlock();
}

template<class T> void ThreadPool<T>::start()
{
  std::for_each(threads.begin(), threads.end(), std::mem_fun(&Thread<T>::start));
}

#endif // THREAD_POOL_HPP
