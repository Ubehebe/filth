#ifndef LOCKS_HPP
#define LOCKS_HPP

#include <pthread.h>

class CondVar;

class Mutex
{
  // No copying, no assigning.
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
  // No copying, no assigning.
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
  // No copying, no assigning.
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

#endif // LOCKS_HPP
