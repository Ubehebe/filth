#include <algorithm>
#include <iostream>
#include <list>
#include <stdlib.h>
#include <syslog.h>

#include "Factory.hpp"
#include "Locks.hpp"
#include "logging.h"
#include "LockFreeQueue.hpp"
#include "ThreadPool.hpp"

using namespace std;

class testobj
{
  static Mutex m;
  static list<testobj *> objs;
  bool touched;
public:
  testobj() : touched(false) { m.lock(); objs.push_back(this); m.unlock(); }
  void touch()
  {
    if (touched) {
      cerr << _SRC"FAILED" << endl;
      abort();
    }
    touched = true;
  }
  void verify()
  {
    if(!touched) {
      cerr << _SRC"FAILED" << endl;
      abort();
    }
  }
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

template<> class Factory<testproducer>
{
public:
  testproducer *operator()() { return new testproducer; }
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

template<> class Factory<testconsumer>
{
public:
  testconsumer *operator()() { return new testconsumer; }
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

  Factory<testproducer> pfact;
  Factory<testconsumer> cfact;

  ThreadPool<testproducer> *ppool
    = new ThreadPool<testproducer>(pfact, &testproducer::produce, nproducers);
  ThreadPool<testconsumer> *cpool
    = new ThreadPool<testconsumer>(cfact, &testconsumer::consume, nconsumers);
  ppool->start();
  cpool->start();
  cerr << "\twaiting for producers to finish\n";
  delete ppool;
  cerr << "\tenqueuing null item to stop consumers\n";
  q->enq(NULL);
  cerr << "\twaiting for consumers to finish\n";
  delete cpool;
  testobj *last;
  cerr << "\tverifying that the null item is the only thing in the queue\n";
  if (!q->nowait_deq(last)) {
    cerr << _SRC"FAILED" << endl;
    abort();
  }
  if (!(last == NULL)) {
    cerr << _SRC"FAILED" << endl;
    abort();
  }
  if (q->nowait_deq(last)) {
    cerr << _SRC"FAILED" << endl;
    abort();
  }
  cerr << "\tverifying that every item enqueued was dequeued exactly once\n";
  testobj::cleanup();
}

int main(int argc, char **argv)
{
  openlog(argv[0], LOG_PERROR, LOG_USER);
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

  LockFreeQueue<testobj *> q;
  test(&q, nproducers, nconsumers, nreps);
  cerr << "passed\n";
}
