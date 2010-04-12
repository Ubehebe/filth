#ifndef WORK_HPP
#define WORK_HPP

#include <iostream>
#include <stdint.h>

#include "Callback.hpp"

/* Allocating/deallocating pieces of work is expected to be the major memory
 * management activity of a server, so in many cases it makes sense to
 * overload operator new and operator delete.
 *
 * TODO: find a way to enforce in the base class, providing sane defaults. */
class Work : public Callback
{
public:
  enum mode { read, write } m;
  int fd;
  bool deleteme;
  Work(int fd, mode m, bool closeme=false);
  virtual ~Work();
  int rduntil(std::ostream &inbuf, uint8_t *rdbuf, size_t rdbufsz);
  int rduntil(std::string &s, uint8_t *rdbuf, size_t rdbufsz, size_t &tord);
  int wruntil(uint8_t const *&outbuf, size_t &towrite);
private:
  Work(Work const&);
  Work &operator=(Work const&);
};

#endif // WORK_HPP
