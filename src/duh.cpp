#include <iostream>
#include <list>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define _LOG_DEBUG

#include "SigThread.hpp"

using namespace std;

class Deadlock
{
  Mutex m;
public:
  Deadlock() { m.lock(); }
  ~Deadlock() { m.unlock(); }
  void deadlock() { m.lock(); }
};

int main()
{
  openlog("duh", LOG_PERROR, LOG_USER);
  SigThread<Deadlock>::setup(SIGINT, SIGUSR1);
  while (true)  {
    Deadlock d[10];
    for (int i=0; i<10; ++i)
      SigThread<Deadlock>(&d[i], &Deadlock::deadlock);
    SigThread<Deadlock>::wait();
  }
}

