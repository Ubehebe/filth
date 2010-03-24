#include <errno.h>
#include <iostream>
#include <unistd.h>

#include "logging.h"
#include "ServerErrs.hpp"
#include "Worker.hpp"

using namespace std;

ConcurrentQueue<Work *> *Worker::q = NULL;

void Worker::work()
{
  _LOG_INFO("worker commencing");

  while (true) {
    Work *w = q->wait_deq();
    // a NULL Work object means stop!
    if (w == NULL) {
      // Put it back, for the other workers to see
      q->enq(w);
      break;
    } else {
      try {
	(*w)();
      }
      // ?????
      catch (SocketErr e) {
	w->deleteme = true;
      }
      if (w->deleteme) {
	/* Purely for debugging with overloaded operator delete. If operator
	   delete sees deleteme == true, it's already been deleted = a bug. */
	w->deleteme = false;
	delete w;
      }
    }
  }
  _LOG_INFO("worker retiring");
}
