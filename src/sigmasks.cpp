#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include "logging.h"
#include "sigmasks.hpp"

using namespace std;

void sigmasks::sigmask_caller(int how, int sig)
{
  sigset_t s;
  if (sigemptyset(&s)==-1) {
    _LOG_FATAL("sigemptyset: %m");
    exit(1);
  }
  if (sigaddset(&s, sig)==-1) {
    _LOG_FATAL("sigaddset: %m");
    exit(1);
  }
  if (pthread_sigmask(how, &s, NULL) != 0) {
    _LOG_FATAL("pthread_sigmask: %m");
    exit(1);
  }
}

void sigmasks::sigmask_caller(int how, vector<int> &sigs)
{
  sigset_t s;
  if (sigemptyset(&s)==-1) {
    _LOG_FATAL("sigemptyset: %m");
    exit(1);
  }
  for (vector<int>::iterator it = sigs.begin(); it != sigs.end(); ++it) {
    if (sigaddset(&s, *it)==-1) {
      _LOG_FATAL("sigaddset: %m");
      exit(1);
    }
  }
  if (pthread_sigmask(how, &s, NULL) != 0) {
    _LOG_FATAL("pthread_sigmask: %m");
    exit(1);
  }
}

void sigmasks::sigmask_caller(int how, sigset_t *sigset)
{
  if (sigset == NULL)
    return;
  if (pthread_sigmask(how, sigset, NULL) != 0) {
    _LOG_FATAL("pthread_sigmask: %m");
    exit(1);
  }
}

void sigmasks::sigmask_caller(builtin b)
{
  sigset_t s;
  if (b == sigmasks::BLOCK_NONE
      && sigemptyset(&s)==-1) {
      _LOG_FATAL("sigemptyset: %m");
      exit(1);
  }
  else if (b == sigmasks::BLOCK_ALL
	   && sigfillset(&s)==-1) {
    _LOG_FATAL("sigfillset: %m");
    exit(1);
  }
  if (pthread_sigmask(SIG_SETMASK, &s, NULL) != 0) {
    _LOG_FATAL("pthread_sigmask: %m");
    exit(1);
  }
}
