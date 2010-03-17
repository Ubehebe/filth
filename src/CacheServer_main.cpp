#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "CacheServer.hpp"
#include "logging.h"

void logatexit()
{
  _LOG_INFO("exiting");
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    printf("usage: %s <mountpath>\n", argv[0]);
    exit(0);
  }
  openlog((argv[0][0] == '.' && argv[0][1] == '/') ? &argv[0][2] : argv[0],
	  SYSLOG_OPTS, LOG_USER);
  atexit(logatexit);

  CacheServer(argv[1], 10, 1, 10, 10);
}
