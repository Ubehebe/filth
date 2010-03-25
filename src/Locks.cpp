#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "Locks.hpp"
#include "logging.h"

/* N.B. none of the destructors should cause fatal errors because I want to use
 * asynchronous thread cancelation as a last-ditch deadlock escape hatch.
 * If a thread is canceled while it's holding a lock, we want to note that but
 * move on; ideally the lock will be destroyed and reconstructed in the next
 * iteration of a containing loop. */

Mutex::Mutex()
{
  pthread_mutex_init(&_m, NULL);
}

Mutex::~Mutex()
{
  if ((errno = pthread_mutex_destroy(&_m))!=0)
    _LOG_INFO("pthread_mutex_destroy: %m");
}

void Mutex::lock()
{
  if ((errno = pthread_mutex_lock(&_m))!=0) {
    _LOG_FATAL("pthread_mutex_lock: %m");
    exit(1);
  }
}

void Mutex::unlock()
{
  if ((errno = pthread_mutex_unlock(&_m))!=0) {
    _LOG_FATAL("pthread_mutex_unlock: %m");
    exit(1);
  }
}

CondVar::CondVar(Mutex &m) : _m(m._m)
{
  if ((errno = pthread_cond_init(&_c, NULL))!=0) {
    _LOG_FATAL("pthread_cond_init: %m");
    exit(1);
  }
}

void CondVar::wait()
{
  if ((errno = pthread_cond_wait(&_c, &_m))!=0) {
    _LOG_FATAL("pthread_cond_wait: %m");
    exit(1);
  }
}

void CondVar::signal()
{
  if ((errno = pthread_cond_signal(&_c))!=0) {
    _LOG_FATAL("pthread_cond_signal: %m");
    exit(1);
  }
}

void CondVar::broadcast()
{
  if ((errno = pthread_cond_broadcast(&_c))!=0) {
    _LOG_FATAL("pthread_cond_broadcast: %m");
    exit(1);
  }
}

CondVar::~CondVar()
{
  if ((errno = pthread_cond_destroy(&_c))!=0)
    _LOG_INFO("pthread_cond_destroy: %m");
}

RWLock::RWLock()
{
  if ((errno = pthread_rwlock_init(&_l, NULL))!=0) {
    _LOG_FATAL("pthread_rwlock_init: %m");
    exit(1);
  }
}

RWLock::~RWLock()
{
  if ((errno = pthread_rwlock_destroy(&_l))!=0)
    _LOG_INFO("pthread_rwlock_destroy: %m");
}

void RWLock::rdlock()
{
  if ((errno = pthread_rwlock_rdlock(&_l))!=0) {
    _LOG_FATAL("pthread_rwlock_rdlock: %m");
    exit(1);
  }
}

void RWLock::wrlock()
{
  if ((errno = pthread_rwlock_wrlock(&_l))!=0) {
    _LOG_FATAL("pthread_rwlock_wrlock: %m");
    exit(1);
  }
}

void RWLock::unlock()
{
  if ((errno = pthread_rwlock_unlock(&_l))!=0) {
    _LOG_FATAL("pthread_rwlock_unlock: %m");
    exit(1);
  }
}

Semaphore::Semaphore(unsigned int init_val)
{
  if (sem_init(&sem, 0, init_val)==-1) {
    _LOG_FATAL("sem_init: %m");
    exit(1);
  }
}

void Semaphore::up()
{
  if (sem_post(&sem)==-1) {
    _LOG_FATAL("sem_post: %m");
    exit(1);
  }
}

void Semaphore::down()
{
  while (sem_wait(&sem)==-1) {
    if (errno == EINTR)
      continue;
    else {
      _LOG_FATAL("sem_wait: %m");
      exit(1);
    }
  }
}

int Semaphore::val()
{
  int ans;
  if (sem_getvalue(&sem, &ans)==-1) {
    _LOG_FATAL("sem_getvalue: %m");
    exit(1);
  }
  return ans;
}

Semaphore::~Semaphore()
{
  if (sem_destroy(&sem)==-1) {
    _LOG_FATAL("sem_destroy: %m");
    exit(1);
  }
}
