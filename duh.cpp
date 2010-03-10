#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>

int main()
{
  struct rlimit rlim;
  rlim.rlim_cur = 1<<12;
  rlim.rlim_max = 1<<12;
  setrlimit(RLIMIT_AS, &rlim);
  try {
  char *foo = new char[1<<13];
  }
  catch (std::bad_alloc) {
    std::cout << "whoops!\n";
  }
}
