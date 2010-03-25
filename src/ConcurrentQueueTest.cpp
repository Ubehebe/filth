#include <algorithm>
#include <assert.h>
#include <iostream>
#include <list>
#include <stdlib.h>

#include "SingleLockedQueue.hpp"
#include "DoubleLockedQueue.hpp"
#include "Locks.hpp"
#include "Thread.hpp"

using namespace std;

class testobj
{
  static Mutex m;
  static list<testobj *> objs;
  bool touched;
public:
  testobj() : touched(false) { m.lock(); objs.push_back(this); m.unlock(); }
  void touch() { assert(!touched); touched = true; }
  void verify() { assert(touched); }
  void deleteme() { delete this; }
  static void cleanup()
  {
    m.lock();
    for_each(objs.begin(), objs.end(), mem_fun(&testobj::verify));
    for_each(objs.begin(), objs.end(), mem_fun(&testobj::deleteme));
    objs.clear();
    m.unlock();
  }
};

class testproducer
{
public:
  static int nreps;
  static ConcurrentQueue<testobj *> *q;
  void produce()
  {
    for (int i=0; i<nreps; ++i)
      q->enq(new testobj());
  }
};

class testconsumer
{
public:
  static ConcurrentQueue<testobj *> *q;
  void consume()
  {
    testobj *t;
    while (true) {
      if ((t = q->wait_deq())!=NULL) {
	t->touch();
      } else {
	q->enq(t);
	break;
      }
    }
  }
};

Mutex testobj::m;
list<testobj *> testobj::objs;
ConcurrentQueue<testobj *> *testconsumer::q = NULL;
ConcurrentQueue<testobj *> *testproducer::q = NULL;
int testproducer::nreps;

void test(ConcurrentQueue<testobj *> *q,
	  int nproducers, int nconsumers, int nreps)
{
  testproducer::q = q;
  testproducer::nreps = nreps;
  testconsumer::q = q;

  testproducer ps[nproducers];
  testconsumer cs[nconsumers];

  list<Thread<testproducer> *> pths;
  list<Thread<testconsumer> *> cths;

  for (int i=0; i<nproducers; ++i)
    pths.push_back(new Thread<testproducer>(&ps[i], &testproducer::produce));
  for (int i=0; i<nconsumers; ++i)
    cths.push_back(new Thread<testconsumer>(&cs[i], &testconsumer::consume));

  cerr << "\twaiting for producers to finish\n";
  for (list<Thread<testproducer> *>::iterator it = pths.begin(); it != pths.end(); ++it)
    delete *it;
  cerr << "\tenqueuing null item to stop consumers\n";
  q->enq(NULL);
  cerr << "\twaiting for consumers to finish\n";
  for (list<Thread<testconsumer> *>::iterator it = cths.begin(); it != cths.end(); ++it)
    delete *it;
  testobj *last;
  cerr << "\tverifying that the null item is the only thing in the queue\n";
  assert(q->nowait_deq(last));
  assert(last == NULL);
  assert(!q->nowait_deq(last));
  cerr << "\tverifying that every item enqueued was dequeued exactly once\n";
  testobj::cleanup();
}

int main(int argc, char **argv)
{
  if (argc !=4) {
    cerr << "usage: " << argv[0] << " <num-producers> <num-consumers> "
      "<items per producer>\n";
    return 1;
  }
  int nproducers, nconsumers, nreps;
  if ((nproducers = atoi(argv[1]))<=0) {
    cerr << "invalid argument " << argv[1] << endl;
    return 1;
  }
  if ((nconsumers = atoi(argv[2]))<=0) {
    cerr << "invalid argument " << argv[2] << endl;
    return 1;
  }
  if ((nreps = atoi(argv[3]))<=0) {
    cerr << "invalid argument " << argv[3] << endl;
    return 1;
  }

  DoubleLockedQueue<testobj *> q;
  cerr << "double-locked queue:\n";
  test(&q, nproducers, nconsumers, nreps);
  cerr << "passed\n";
}
