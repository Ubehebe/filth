#ifndef LOCKS_HPP
#define LOCKS_HPP

#include <pthread.h>
#include <semaphore.h>

/** \brief Thin wrapper around pthread mutexes. */
class Mutex
{
  Mutex(Mutex const &);
  Mutex &operator=(Mutex const &);

  friend class CondVar;

  pthread_mutex_t _m;
 public:
  Mutex();
  /** \brief wrapper for pthread_mutex_lock */
  void lock();
  /** \brief wrapper for pthread_mutex_unlock */
  void unlock();
  ~Mutex();
};

/** \brief Thin wrapper around pthread condition variables. */
class CondVar
{
  CondVar(CondVar const &);
  CondVar &operator=(CondVar const &);

  pthread_mutex_t &_m;
  pthread_cond_t _c;
public:
  /** \param m Mutex to bind to this condition variable */
  CondVar(Mutex &m);
  ~CondVar();
  /** \brief wrapper for pthread_cond_broadcast */
  void broadcast();
  /** \brief wrapper for pthread_cond_signal */
  void signal();
  /** \brief wrapper for pthread_cond_wait */
  void wait();
};

/** \brief Thin wrapper around pthread reader-writer locks. */
class RWLock
{
  RWLock(RWLock const &);
  RWLock &operator=(RWLock const &);
  pthread_rwlockattr_t _at;
  pthread_rwlock_t _l;
public:
  /** \param pref can be PTHREAD_RWLOCK_PREFER_READER_NP,
   * PTHREAD_RWLOCK_PREFER_WRITER_NP,
   * PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP, or
   * PTHREAD_RWLOCK_DEFAULT_NP which, according to my pthread.h,
   * is the same as PTHREAD_RWLOCK_PREFER_READER_NP
   * \warning the "NP"s stand for non-portable! */
  RWLock(int pref=PTHREAD_RWLOCK_DEFAULT_NP);
  /** \brief wrapper for pthread_rwlock_rdlock */
  void rdlock();
  /** \brief wrapper for pthread_rwlock_wrlock */
  void wrlock();
  /** \brief wrapper for pthread_rwlock_unlock */
  void unlock();
  ~RWLock();
};

/** \brief Thin wrapper around posix semaphores. */
class Semaphore
{
  sem_t sem;
public:
  /** \param init_val initial value of the semaphore */
  Semaphore(unsigned int init_val=0);
  /** \brief wrapper for sem_post */
  void up();
  /** \brief wrapper for sem_wait */
  void down();
  /** \brief wrapper for sem_getvalue */
  int val();
  ~Semaphore();
};

#endif // LOCKS_HPP
