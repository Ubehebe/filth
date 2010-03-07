#include <errno.h>
#include <iostream>
#include <unistd.h>

#include "Worker.hpp"

LockedQueue<Work *> *Worker::q = NULL;

void Worker::work()
{
  while (true) {
    Work *w = q->wait_deq();
    // a NULL Work object means stop!
    if (w == NULL) {
      // Put it back, for the other workers to see
      q->enq(w);
      break;
    } else {
      (*w)();
      delete w;
    }
  }
  std::cerr << "worker retiring\n";
}
