#include "HTTP_Work.hpp"

HTTP_Work::HTTP_Work(int fd, Work::mode m)
  : Work(fd, m), stat(HTTP_constants::OK), inhdrs_done(false),
    inhdrs(1+HTTP_constants::num_header), outhdrs_done(false), outhdrs(NULL),
    outbody(NULL), out(NULL), inbody_sz(0), outhdrs_sz(0), outbody_sz(0),
    outsz(0)
{
}

Scheduler *HTTP_Work::sch = NULL;
