#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <algorithm>
#include <list>
#include <signal.h>
#include <string.h>

#include "Callback.hpp"
#include "Factory.hpp"
#include "Locks.hpp"
#include "Thread.hpp"

template<class T> class ThreadPool : public Callback
{
public:
  ThreadPool(Factory<T> &f, void (T::*todo)(), int nthreads, int sigkill=-1,
	     std::list<specific_data> *specifics=NULL);
  ~ThreadPool();
  void register_specific(pthread_key_t *k, void *(*constructor)(),
			 void (*destructor)(void *));
  static void UNSAFE_emerg_yank(int ignore=-1);
  void start();
  void operator()(); // cleanup callback
  static ThreadPool<T> *thethreadpool;
private:
  Mutex m;
  CondVar done;
  std::list<Thread<T> *> threads;
  std::list<specific_data> specifics;
  static void pthread_exit_wrapper(int ignore) { (*thethreadpool)(); pthread_exit(NULL); }
  int sigkill;
};

template<class T> ThreadPool<T> *ThreadPool<T>::thethreadpool;

template<class T> ThreadPool<T>::ThreadPool(Factory<T> &f,
					    void (T::*todo)(), int nthreads, int sigkill,
					    std::list<specific_data> *specifics)
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

  if (specifics != NULL) {
    for (std::list<specific_data>::iterator it = specifics->begin();
	 it != specifics->end(); ++it)
      if ((errno = pthread_key_create(it->k, it->destructor)) != 0) {
	_LOG_FATAL("pthread_key_create: %m");
	exit(1);
      }
  }

  m.lock();
  for (int i=0; i<nthreads; ++i)
    threads.push_back(new Thread<T>(f, todo, &toblock, true, this, specifics));
  m.unlock();
}

/* There's a suggestive bug going on here: although every thread
 * in the pool receives the signal, only ONE thread executes this function.
 * The result is deadlock because the ThreadPool destructor is waiting for
 * the thread count to dwindle to zero. Why does this happen? */
template<class T> void ThreadPool<T>::operator()()
{
  typename std::list<Thread<T> *>::iterator it, toerase = threads.end();
  pthread_t me = pthread_self();
  m.lock();
  for (it = threads.begin(); it != threads.end(); ++it) {
    if (pthread_equal(me, (*it)->th)) {
      delete *it; // By this time we shouldn't be using any of this...right?
      toerase = it;
    }
  }
  if (toerase != threads.end()) {
    threads.erase(toerase);
    if (threads.empty())
      done.signal();
  }
  else
    _LOG_INFO("exiting thread %ld didn't find itself in thread pool", me);
    
  m.unlock();
}

template<class T> ThreadPool<T>::~ThreadPool()
{
  m.lock();
  while (!threads.empty()) {
    done.wait();
  }
  m.unlock();
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
      if ((errno = pthread_kill((*it)->th, thethreadpool->sigkill))!=0)
	_LOG_INFO("pthread_kill: %m, continuing");
    }
    thethreadpool->m.unlock();
  }
}

template<class T> void ThreadPool<T>::register_specific(pthread_key_t *k,
							void *(*constructor)(),
							void (*destructor)(void *))
{
  specifics.push_back(specific_data(k, constructor, destructor));
}

#endif // THREAD_POOL_HPP
