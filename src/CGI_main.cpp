#include "Unix_Domain_Server.h"

// Global for signal handlers.
Unix_Domain_Server *serv;

void handle_SIGINT(int signum)
{
  std::cerr << "got SIGINT, shutting down.\n"
	    << "Send another SIGINT to exit immediately.\n";
  delete serv;
}

void at_exit()
{
  delete serv;
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    printf("usage: %s <domain socket name>\n", argv[0]);
    _exit(1);
  }

  serv = new Unix_Domain_Server(argv[1], handle_SIGINT, at_exit);
  try {
    (*serv)();
  }
  catch (SocketErr e) {
    errno = e.err;
    perror(e.msg);
    exit(1);
  }
  catch (ResourceErr e) {
    errno = e.err;
    perror(e.msg);
    exit(1);
  }
	
  /* Not exit(1); that would cause the server destructor to be called, but
   * if we're here, we have already called it. */
  _exit(1);
}
