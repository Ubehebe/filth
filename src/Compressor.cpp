#include <stdlib.h>
#include <string.h>

#include "Compressor.hpp"
#include "logging.h"

Compressor::Compressor(int level)
{
  // Use default memory allocation
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  if (deflateInit(&strm, level) != Z_OK) {
    _LOG_FATAL("deflateInit");
    exit(1);
  }
}

Compressor::~Compressor()
{
  deflateEnd(&strm);
}

char *Compressor::operator()(char *dst, char *src, size_t sz)
{
  m.lock();

  char *toret = dst;
  strm.avail_in = strm.avail_out = sz;
  strm.next_in = reinterpret_cast<Bytef *>(src);
  
  size_t ndone;
  do {
    strm.next_out = reinterpret_cast<Bytef *>(dst);
    deflate(&strm, Z_FINISH);
    ndone = sz - strm.avail_out;
    memmove(reinterpret_cast<void *>(dst),
	    reinterpret_cast<void *>(src), ndone); // OMG will this work?
    dst += ndone;
  } while (strm.avail_out == 0);

  m.unlock();

  return toret;
}
