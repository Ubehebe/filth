#ifndef WORK_HPP
#define WORK_HPP

#include <iostream>
#include <stdint.h>

class Work
{
public:
  enum mode { read, write } m;
  int fd;
  bool deleteme;
  Work(int fd, mode m, bool closeme=false);
  virtual void operator()() = 0; // The only function seen by the worker
  virtual ~Work();
protected:
  int rduntil(std::ostream &inbuf, uint8_t *rdbuf, size_t rdbufsz);
  int rduntil(std::string &s, uint8_t *rdbuf, size_t rdbufsz, size_t &tord);
  int wruntil(uint8_t const *&outbuf, size_t &towrite);
private:
  Work(Work const&);
  Work &operator=(Work const&);
};

#endif // WORK_HPP
