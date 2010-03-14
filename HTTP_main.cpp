#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "HTTP_Server.hpp"

using namespace HTTP_cmdline;

void logatexit()
{
  _LOG_INFO("exiting");
}

int main(int argc, char **argv)
{
  openlog((argv[0][0] == '.' && argv[0][1] == '/') ? &argv[0][2] : argv[0],
	  LOG_PERROR, LOG_USER);
  atexit(logatexit);
  _LOG_INFO("starting");
  _LOG_DEBUG("sizeof(Server)=%d", sizeof(Server));
  _LOG_DEBUG("sizeof(HTTP_Server)=%d", sizeof(HTTP_Server));
  _LOG_DEBUG("sizeof(Scheduler)=%d", sizeof(Scheduler));
  _LOG_DEBUG("sizeof(Worker)=%d", sizeof(Worker));
  _LOG_DEBUG("sizeof(Work)=%d", sizeof(Work));
  _LOG_DEBUG("sizeof(HTTP_Work)=%d", sizeof(HTTP_Work));
  _LOG_DEBUG("sizeof(Thread<Scheduler>)=%d", sizeof(Thread<Scheduler>));
  _LOG_DEBUG("sizeof(Thread<Worker>)=%d", sizeof(Thread<Worker>));
  parsecmdline(argc, argv);
  try {
    HTTP_Server(
		HTTP_cmdline::svals[HTTP_cmdline::port],
		HTTP_cmdline::svals[HTTP_cmdline::ifnam],
		HTTP_cmdline::svals[HTTP_cmdline::mount],
		HTTP_cmdline::ivals[HTTP_cmdline::nworkers],
		HTTP_cmdline::bvals[HTTP_cmdline::ipv6],
		HTTP_cmdline::ivals[HTTP_cmdline::cachesz]).serve();
  }
  catch (ResourceErr e) {
    _LOG_CRIT("uncaught ResourceErr: %s: %s", e.msg, strerror(e.err));
    exit(1);
  }
  catch (SocketErr e) {
    _LOG_CRIT("uncaught SocketErr: %s: %s", e.msg, strerror(e.err));
    exit(1);
  }
}
