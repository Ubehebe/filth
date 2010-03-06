#include <iostream>
#include "HTTP_Work.h"

LockedQueue<Work *> *HTTP_Work::q = NULL;
Scheduler *HTTP_Work::sch = NULL;

void HTTP_Work::operator()()
{
  std::cout << "boing! nothing to do yet\n";
}

void HTTP_mkWork::init(LockedQueue<Work *> *q, Scheduler *sch)
{
  HTTP_Work::q = q;
  HTTP_Work::sch = sch;
}

Work *HTTP_mkWork::operator()(int fd, Work::mode m)
{
  return new HTTP_Work(fd, m);
}



