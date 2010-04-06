#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include <zlib.h>

#include "Locks.hpp"

class Compressor
{
public:
  Compressor(int level=Z_DEFAULT_COMPRESSION);
  ~Compressor();
  char *operator()(char *dst, char *src, size_t sz);
private:
  Mutex m;
  z_stream strm;
};

#endif // COMPRESSOR_HPP
