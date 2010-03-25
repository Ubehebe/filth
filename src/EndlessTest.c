#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PROGNAME "./concurrent-queue-test"
#define BUFSZ 10

int main(int argc, char **argv)
{
  srand(time(NULL));
  int status;
  char *myargv[5];
  char nproducers[BUFSZ], nconsumers[BUFSZ], nreps[BUFSZ];
  myargv[0] = PROGNAME;
  myargv[1] = nproducers;
  myargv[2] = nconsumers;
  myargv[3] = nreps;
  myargv[4] = NULL;
  
  while (1) {
    snprintf(nproducers, BUFSZ, "%d", (rand() % 50) + 1);
    snprintf(nconsumers, BUFSZ, "%d", (rand() % 50) + 1);
    snprintf(nreps, BUFSZ, "%d", (rand() % 10000) + 1);
    fprintf(stderr, "try %s producers %s consumers %s items/producer...",
	    nproducers, nconsumers, nreps);
    // Parent
    if (fork()) {
      wait(&status);
      fprintf(stderr, "%s\n", strerror(status));
      sleep(10);
    }
    // Child
    else {
      execve(PROGNAME, myargv, NULL);
    }
  }
}
