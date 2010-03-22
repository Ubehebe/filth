#include <iostream>
#include <list>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define _LOG_DEBUG

#include "Locks.hpp"
#include "sigmasks.hpp"
#include "Thread.hpp"

using namespace std;

class Deadlock
{
  Mutex m;
public:
  Deadlock() { m.lock(); }
  ~Deadlock() { m.unlock(); }
  void deadlock() { m.lock(); }
};

void hopeful(int ignore)
{
  Thread<Deadlock>::sigall(0);
}

void hopeful2(int ignore)
{
  pthread_exit(NULL);
}

int main()
{
  sigmasks::sigmask_caller(sigmasks::BLOCK_ALL);
  sigmasks::sigmask_caller(SIG_UNBLOCK, SIGINT);
  Thread<Deadlock>::setup_emerg_exitall(SIGCONT);
  struct sigaction act;
  memset((void *)&act, 0, sizeof(act));
  act.sa_handler = hopeful;
  if (sigaction(SIGINT, &act, NULL)==-1)
    _LOG_DEBUG("sigaction: %m");

  openlog("duh", LOG_PERROR, LOG_USER);
  while (true)  {
    list<Thread<Deadlock> *> l;
    for (int i=0; i<10; ++i)
      l.push_back(new Thread<Deadlock>(&Deadlock::deadlock));
    Thread<Deadlock> *th;
    while (!l.empty()) {
      th = l.front();
      l.pop_front();
      delete th;
    }
  }
}

