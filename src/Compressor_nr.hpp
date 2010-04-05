#ifndef COMPRESSOR_NR_HPP
#define COMPRESSOR_NR_HPP

#include <zlib.h>

class Compressor_nr
{
public:
  Compressor_nr(int level=Z_DEFAULT_COMPRESSION);
  ~Compressor_nr();
  char *operator()(char *dst, char *src, size_t sz);
private:
  z_stream strm;
};

#endif // COMPRESSOR_NR_HPP
