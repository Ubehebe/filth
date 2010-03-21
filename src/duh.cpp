#include <iostream>

#include "Locks.hpp"
#include "sigmasks.hpp"
#include "Thread.hpp"

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
  Deadlock d;
  Thread<Deadlock> th(&d, &Deadlock::deadlock,
		      PTHREAD_CANCEL_ENABLE, PTHREAD_CANCEL_ASYNCHRONOUS);
  th.cancel();
}

