#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
  while (true) {
    pid_t pid;
    if (pid = fork()) {
      wait(NULL);
      sleep(60); // For TCP time_wait
    } else {
      char *const _argv[] = { "./filth", "-m/home/brendan", "-ilo", "-p82", NULL };
      char *const _envp[] = { NULL };
      execve("./filth", _argv, _envp);
    }
  }
}
