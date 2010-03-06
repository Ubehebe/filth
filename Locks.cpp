#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "Locks.h"

Mutex::Mutex()
{
  pthread_mutex_init(&_m, NULL);
}

Mutex::~Mutex()
{
  if ((errno = pthread_mutex_destroy(&_m))!=0) {
    perror("pthread_mutex_destroy");
    abort();
  }
}

void Mutex::lock()
{
  if ((errno = pthread_mutex_lock(&_m))!=0) {
    perror("pthread_mutex_lock");
    abort();
  }
}

void Mutex::unlock()
{
  if ((errno = pthread_mutex_unlock(&_m))!=0) {
    perror("pthread_mutex_unlock");
    abort();
  }
}

CondVar::CondVar(Mutex &m) : _m(m._m)
{
  if ((errno = pthread_cond_init(&_c, NULL))!=0) {
    perror("pthread_cond_init");
    abort();
  }
}

void CondVar::wait()
{
  if ((errno = pthread_cond_wait(&_c, &_m))!=0) {
    perror("pthread_cond_wait");
    abort();
  }
}

void CondVar::signal()
{
  if ((errno = pthread_cond_signal(&_c))!=0) {
    perror("pthread_cond_signal");
    abort();
  }
}

void CondVar::broadcast()
{
  if ((errno = pthread_cond_broadcast(&_c))!=0) {
    perror("pthread_cond_broadcast");
    abort();
  }
}

CondVar::~CondVar()
{
  if ((errno = pthread_cond_destroy(&_c))!=0) {
    perror("pthread_cond_destroy");
    abort();
  }
}

RWLock::RWLock()
{
  if ((errno = pthread_rwlock_init(&_l, NULL))!=0) {
    perror("pthread_rwlock_init");
    abort();
  }
}

RWLock::~RWLock()
{
  if ((errno = pthread_rwlock_destroy(&_l))!=0) {
    perror("pthread_rwlock_destroy");
    abort();
  }
}

void RWLock::rdlock()
{
  if ((errno = pthread_rwlock_rdlock(&_l))!=0) {
    perror("pthread_rwlock_rdlock");
    abort();
  }
}

void RWLock::wrlock()
{
  if ((errno = pthread_rwlock_wrlock(&_l))!=0) {
    perror("pthread_rwlock_wrlock");
    abort();
  }
}

void RWLock::unlock()
{
  if ((errno = pthread_rwlock_unlock(&_l))!=0) {
    perror("pthread_rwlock_unlock");
    abort();
  }
}

