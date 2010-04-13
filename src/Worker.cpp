#include <string.h>

#include "logging.h"
#include "ServerErrs.hpp"
#include "Worker.hpp"

using namespace std;

ConcurrentQueue<Work *> *Worker::jobq = NULL;

void Worker::work()
{
  _LOG_INFO("worker commencing");

  while (true) {
    Work *w = jobq->wait_deq();
    // a NULL Work object means stop!
    if (w == NULL) {
      // Put it back, for the other workers to see
      jobq->enq(w);
      break;
    }
    else if (w->deleteme) {
      delete w;
    }
    else {
      try {
	(*w)(this);
      }
      catch (SocketErr e) {
	_LOG_INFO("%s (%s), closing socket %d", e.msg, strerror(e.err), w->fd);
	w->deleteme = true;
      }
      if (w->deleteme)
	delete w;
    }
  }
  _LOG_INFO("worker retiring");
}
