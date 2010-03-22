#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>

int main(int argc, char **argv)
{
  if (argc != 3) {
    printf("usage: %s tgid tid\n", argv[0]);
    return 0;
  }
  int tgid = atoi(argv[1]);
  int tid = atoi(argv[2]);
  int err;
  if ((err=syscall(SYS_tgkill, tgid, tid, SIGUSR2))==-1)
    perror("tgkill:");
  return err;
}
