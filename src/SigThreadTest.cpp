#include <assert.h>
#include <semaphore.h>
#include <iostream>
#include <list>

#include "Locks.hpp"
#include "SigThread.hpp"

using namespace std;

class counter
{
  Semaphore &s;
public:
  static int nreps;
  counter(Semaphore &s) : s(s) {}
  void count()
  {
    for (int i=0; i<nreps; ++i)
      s.up();
  }
};

int counter::nreps;

int main(int argc, char **argv)
{
  openlog(argv[0], LOG_PERROR, LOG_USER);
  if (argc != 3) {
    cerr << "usage: " << argv[0] << " <nthreads> <nreps>\n";
    return 1;
  }
  int nthreads;
  if ((nthreads = atoi(argv[1])) <= 0) {
    cerr << "invalid argument " << argv[1] << endl;
    return 1;
  }
  if ((counter::nreps = atoi(argv[2])) <= 0) {
    cerr << "invalid argument " << argv[2] << endl;
    return 1;
  }
  Semaphore sem;
  counter *cs[nthreads];
  SigThread<counter>::setup();
  for (int i=0; i<nthreads; ++i) {
    cs[i] = new counter(sem);
    SigThread<counter>(cs[i], &counter::count);
  }
  SigThread<counter>::wait();
  cerr << "got " << sem.val() << " expected " << nthreads * counter::nreps << endl;
  return 1-(sem.val() == nthreads * counter::nreps);
}
