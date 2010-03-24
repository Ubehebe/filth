#include <algorithm>
#include <assert.h>
#include <iostream>
#include <list>

#include "ConcurrentQueue.hpp"
#include "SingleLockedQueue.hpp"
#include "DoubleLockedQueue.hpp"
#include "LockFreeQueue.hpp"
#include "Locks.hpp"
#include "SigThread.hpp"

using namespace std;

class testobj
{
  static Mutex m;
  static list<testobj *> objs;
  bool touched;
public:
  testobj() : touched(false) { m.lock(); objs.push_back(this); m.unlock(); }
  void touch() { /*assert(!touched); touched = true;*/ }
  void verify() { /*assert(touched); */ }
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
  int n;
public:
  static ConcurrentQueue<testobj *> *q;
  testproducer(int n=100)
    : n(n) {}
  void produce()
  {
    for (int i=0; i<n; ++i)
      q->enq(new testobj());
    q->enq(NULL);
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

void test(ConcurrentQueue<testobj *> &q, int nproducers, int nconsumers)
{
  cout << nproducers << " producers, " << nconsumers << " consumers\n";
  testproducer::q = &q;
  testconsumer::q = &q;

  testproducer ps[nproducers];
  testconsumer cs[nconsumers];

  for (int i=0; i<nproducers; ++i)
    SigThread<testproducer>(&ps[i], &testproducer::produce);
  for (int i=0; i<nconsumers; ++i)
    SigThread<testconsumer>(&cs[i], &testconsumer::consume);
  SigThread<testproducer>::wait();
  SigThread<testconsumer>::wait();
  testobj *last;
  //  assert(q.nowait_deq(last));
  //  assert(last == NULL);
  //  assert(!q.nowait_deq(last));
  testobj::cleanup();
}

int main()
{
  SingleLockedQueue<testobj *> q1;
  DoubleLockedQueue<testobj *> q2;
  LockFreeQueue<testobj *> q3;
  test(q2, 10, 1);
}
