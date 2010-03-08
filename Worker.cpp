#include <errno.h>
#include <iostream>
#include <unistd.h>

#include "sigmasks.hpp"
#include "Worker.hpp"

LockedQueue<Work *> *Worker::q = NULL;

void Worker::work()
{
  // A worker should never have to deal with signal handling...right?
  sigmasks::sigmask_caller(sigmasks::BLOCK_ALL);

  while (true) {
    Work *w = q->wait_deq();
    // a NULL Work object means stop!
    if (w == NULL) {
      // Put it back, for the other workers to see
      q->enq(w);
      break;
    } else {
      (*w)();
      if (w->deleteme)
	delete w;
    }
  }
  std::cerr << "worker retiring\n";
}
