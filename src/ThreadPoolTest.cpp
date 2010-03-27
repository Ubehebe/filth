#include <iostream>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "Factory.hpp"
#include "Locks.hpp"
#include "ThreadPool.hpp"

using namespace std;

class test
{
public:
  static int *victim;
  static Mutex *m;
  static int nreps;
  void work()
  {
    for (int i=0; i<nreps; ++i) {
      m->lock();
      (*victim)++;
      m->unlock();
    }
  }
};

class fake
{
public:
  void work()
  {
    sleep(15);
  }
};

int *test::victim = NULL;
int test::nreps = 0;
Mutex *test::m = NULL;

template<> class Factory<test>
{
public:
  Factory(int &victim, Mutex &m, int nreps)
  {
    test::victim = &victim;
    test::m = &m;
    test::nreps = nreps;
  }
  test *operator()()
  {
    return new test();
  }
};

int main(int argc, char **argv)
{
  int nloops = (argc > 1) ? atoi(argv[1]) : -1;
  int loops = 0;
  srand(time(NULL));
  int victim, nthreads, nreps;
  useconds_t usecs;
  Mutex m;
  fake *fk = new fake();
  Thread<fake> faketh(fk, &fake::work);
  faketh.start();
  while (nloops == -1 || loops++ < nloops) {
    nthreads = (rand() % 50) + 1;
    nreps = (rand() % 100000) + 1;
    usecs = rand() % 5000000;
    cerr << nthreads << " threads, "
	 << nreps << " attempts per thread; sleeping "
	 << usecs << " usecs before yanking\n";
    victim = 0;
    Factory<test> f(victim, m, nreps);
    ThreadPool<test> *pool 
      = new ThreadPool<test>(f, &test::work, nthreads, SIGUSR1);
    pool->start();
    usleep(usecs);
    m.lock();
    int before = victim;
    pool->UNSAFE_emerg_yank();
    m.unlock();
    delete pool;
    cerr << "before " << before << " after " << victim
	 << ((before == victim) ? " (ok)\n" : "(FAILURE)\n");
    sleep(10);
  }
}
