#include <algorithm>
#include <assert.h>
#include <iostream>
#include <list>
#include <stdlib.h>

#include "ConcurrentQueue.hpp"
#include "SingleLockedQueue.hpp"
#include "DoubleLockedQueue.hpp"
#include "LockFreeQueue.hpp"
#include "Locks.hpp"
#include "logging.h"
#include "NonThreadSafeQueue.hpp"
#include "SigThread.hpp"

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

  SigThread<testproducer>::setup();
  SigThread<testconsumer>::setup();

  /* Start the consumers before the producers so we don't miss a
   * signal on a condition variable, depending on the implementation. */
  for (int i=0; i<nconsumers; ++i)
    SigThread<testconsumer>(&cs[i], &testconsumer::consume);
  for (int i=0; i<nproducers; ++i)
    SigThread<testproducer>(&ps[i], &testproducer::produce);
  cout << "\twaiting for producers to finish\n";
  SigThread<testproducer>::wait();
  q->enq(NULL);
  cout << "\twaiting for consumers to finish\n";
  SigThread<testconsumer>::wait();
  testobj *last;
  assert(q->nowait_deq(last));
  assert(last == NULL);
  assert(!q->nowait_deq(last));
  testobj::cleanup();
}

int main(int argc, char **argv)
{
  if (argc !=4) {
    cout << "usage: " << argv[0] << " <num-producers> <num-consumers> "
      "<items per producer>\n";
    return 0;
  }
  openlog(argv[0], LOG_PERROR, LOG_USER);
  int nproducers, nconsumers, nreps;
  if ((nproducers = atoi(argv[1]))<=0) {
    cout << "invalid argument " << argv[1] << endl;
    return 1;
  }
  if ((nconsumers = atoi(argv[2]))<=0) {
    cout << "invalid argument " << argv[2] << endl;
    return 1;
  }
  if ((nreps = atoi(argv[3]))<=0) {
    cout << "invalid argument " << argv[3] << endl;
    return 1;
  }

  SingleLockedQueue<testobj *> q1;
  DoubleLockedQueue<testobj *> q2;
  LockFreeQueue<testobj *> q3;
  NonThreadSafeQueue<testobj *> q4;

  list<pair<ConcurrentQueue<testobj *> *, string> > qs;
  qs.push_back(make_pair(&q1, "single locked queue"));
  qs.push_back(make_pair(&q2, "double locked queue"));
  qs.push_back(make_pair(&q3, "lock-free queue"));
  qs.push_back(make_pair(&q4, "plain old STL queue (should assert/segfault)"));
  list<pair<ConcurrentQueue<testobj *> *, string> >::iterator it;
  
  for (it = qs.begin(); it != qs.end(); ++it) {
    cout << it->second << ":\n";
    test(it->first, nproducers, nconsumers, nreps);
    cout << "passed\n";
  }
}
