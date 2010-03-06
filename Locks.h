#ifndef LOCKS_H
#define LOCKS_H

#include <pthread.h>

class CondVar;

class Mutex
{
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
  pthread_rwlock_t _l;
public:
  RWLock();
  void rdlock();
  void wrlock();
  void unlock();
  ~RWLock();
};




#endif // LOCKS_H
