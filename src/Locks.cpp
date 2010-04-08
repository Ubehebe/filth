#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "Locks.hpp"
#include "logging.h"

/* Typically, an error on a synchronization primitive indicates a serious
 * logical flaw, such as when a thread tries to lock a regular mutex
 * twice in a row. The reason none of these functions call exit is because
 * there are rare occasions when it is appropriate to ignore such errors.
 * For example, in a last-ditch de-deadlock mechanism that rips out a thread
 * without regard to its state, a mutex could be destroyed while it is still
 * locked, which would cause an error.
 *
 * According to the syslog man pages, the format string %m is equivalent
 * to strerror(errno), which is not thread-safe. Thus there is no guarantee
 * that the error message is accurate. I'm not concerned enough to hook
 * up strerror_r to the system logger. */

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
  if ((errno = pthread_mutex_lock(&_m))!=0)
    _LOG_INFO("pthread_mutex_lock: %m");
}

void Mutex::unlock()
{
  if ((errno = pthread_mutex_unlock(&_m))!=0)
    _LOG_INFO("pthread_mutex_unlock: %m");
}

CondVar::CondVar(Mutex &m) : _m(m._m)
{
  if ((errno = pthread_cond_init(&_c, NULL))!=0)
    _LOG_INFO("pthread_cond_init: %m");
}

void CondVar::wait()
{
  if ((errno = pthread_cond_wait(&_c, &_m))!=0)
    _LOG_INFO("pthread_cond_wait: %m");
}

void CondVar::signal()
{
  if ((errno = pthread_cond_signal(&_c))!=0)
    _LOG_INFO("pthread_cond_signal: %m");
}

void CondVar::broadcast()
{
  if ((errno = pthread_cond_broadcast(&_c))!=0)
    _LOG_INFO("pthread_cond_broadcast: %m");
}

CondVar::~CondVar()
{
  if ((errno = pthread_cond_destroy(&_c))!=0)
    _LOG_INFO("pthread_cond_destroy: %m");
}

RWLock::RWLock()
{
  if ((errno = pthread_rwlock_init(&_l, NULL))!=0)
    _LOG_INFO("pthread_rwlock_init: %m");
}

RWLock::~RWLock()
{
  if ((errno = pthread_rwlock_destroy(&_l))!=0)
    _LOG_INFO("pthread_rwlock_destroy: %m");
}

void RWLock::rdlock()
{
  if ((errno = pthread_rwlock_rdlock(&_l))!=0)
    _LOG_INFO("pthread_rwlock_rdlock: %m");
}

void RWLock::wrlock()
{
  if ((errno = pthread_rwlock_wrlock(&_l))!=0)
    _LOG_INFO("pthread_rwlock_wrlock: %m");
}

void RWLock::unlock()
{
  if ((errno = pthread_rwlock_unlock(&_l))!=0)
    _LOG_INFO("pthread_rwlock_unlock: %m");
}

Semaphore::Semaphore(unsigned int init_val)
{
  if (sem_init(&sem, 0, init_val)==-1)
    _LOG_INFO("sem_init: %m");
}

void Semaphore::up()
{
  if (sem_post(&sem)==-1)
    _LOG_INFO("sem_post: %m");
}

void Semaphore::down()
{
  while (sem_wait(&sem)==-1) {
    if (errno != EINTR) {
      _LOG_INFO("sem_wait: %m");
      break;
    }
  }
}

int Semaphore::val()
{
  int ans;
  if (sem_getvalue(&sem, &ans)==-1)
    _LOG_INFO("sem_getvalue: %m");
  return ans;
}

Semaphore::~Semaphore()
{
  if (sem_destroy(&sem)==-1)
    _LOG_INFO("sem_destroy: %m");
}
