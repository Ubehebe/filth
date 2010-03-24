#ifndef LOCKS_HPP
#define LOCKS_HPP

#include <pthread.h>
#include <semaphore.h>

// Fwd declaration needed? class CondVar;

class Mutex
{
  Mutex(Mutex const &);
  Mutex &operator=(Mutex const &);

  friend class CondVar;

  pthread_mutex_t _m;
 public:
  Mutex();
  void lock();
  void unlock();
  ~Mutex();
};

class CondVar
{
  CondVar(CondVar const &);
  CondVar &operator=(CondVar const &);

  pthread_mutex_t &_m;
  pthread_cond_t _c;
public:
  CondVar(Mutex &m);
  ~CondVar();
  void broadcast();
  void signal();
  void wait();
};

class RWLock
{
  RWLock(RWLock const &);
  RWLock &operator=(RWLock const &);
  pthread_rwlock_t _l;
public:
  RWLock();
  void rdlock();
  void wrlock();
  void unlock();
  ~RWLock();
};

class Semaphore
{
  sem_t sem;
public:
  Semaphore(unsigned int init_val=0);
  void up();
  void down();
  ~Semaphore();
};

#endif // LOCKS_HPP
