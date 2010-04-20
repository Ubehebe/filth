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

/** \brief The common pattern of instantiating n identical objects,
 * binding each to a separate thread. */
template<class T> class ThreadPool : public Callback
{
public:
  /** \param f how to make instances of class T (this constructor constructs
   * and destroys its own instances)
   * \param todo what each thread executes
   * \param nthreads number of threads to create
   * \param sigkill signal to trigger UNSAFE_emerg_yank(). Default of -1 means
   * UNSAFE_emerg_yank will not happen. */
  ThreadPool(Factory<T> &f, void (T::*todo)(), int nthreads, int sigkill=-1);
  /** \note The destructor waits for all of the threads in the pool to finish.
   * This is handy because you can enclose a thread pool in curly braces
   * and the closing brace will automatically perform the wait. */
  ~ThreadPool();
  /** \brief Asynchronously rip out all the threads.
   * \param ignore dummy argument, only present because of the function
   * pointer requirements of sigaction
   * \warning Use only as a last resort (i.e., you are deadlocked and have
   * nothing to lose). I am absolutely not sure I've implemented this right,
   * and even if I have, there's a good reason why asynchronously destroying
   * threads is not easy. The state associated with the threads could be
   * anything at all: locks might be locked or unlocked, for example. You will
   * probably also leak some memory. */
  static void UNSAFE_emerg_yank(int ignore=-1);
  /** \brief Start all the threads in the pool. */
  void start();

private:
  friend class Thread<T>;
  static ThreadPool<T> *thethreadpool;
  void operator()();
  Mutex m;
  CondVar done;
  std::list<Thread<T> *> threads;
  static void pthread_exit_wrapper(int ignore) { (*thethreadpool)(); pthread_exit(NULL); }
  int sigkill;
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
    threads.push_back(new Thread<T>(f, todo, &toblock, true, this));
  m.unlock();
}

template<class T> void ThreadPool<T>::operator()()
{
  typename std::list<Thread<T> *>::iterator it, toerase = threads.end();
  pthread_t me = pthread_self();
  m.lock();
  for (it = threads.begin(); it != threads.end(); ++it) {
    if (pthread_equal(me, (*it)->getth())) {
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
      if ((errno = pthread_kill((*it)->getth(), thethreadpool->sigkill))!=0)
	_LOG_INFO("pthread_kill: %m, continuing");
    }
    thethreadpool->m.unlock();
  }
}

#endif // THREAD_POOL_HPP
