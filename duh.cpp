#include <iostream>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>


int main(int argc, char **argv)
{
  openlog(argv[0], LOG_PID, LOG_USER);
  if (close(-1) == -1) {
    syslog(LOG_ERR, "close: %m, exiting");
    exit(1);
  }
}
