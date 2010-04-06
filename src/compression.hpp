#ifndef COMPRESSION_HPP
#define COMPRESSION_HPP

#include <stdint.h>
#include <zlib.h>

/* A thin wrapper on top of zlib. The zlib documentation says it is
 * thread-safe. */

namespace compression
{
  bool compress(void *dst, size_t &dstsz, void const *src, size_t srcsz,
		int level=Z_DEFAULT_COMPRESSION);
  void *compress(void const *src, size_t srcsz, size_t &dstsz,
		 int level=Z_DEFAULT_COMPRESSION);
  bool uncompress(void *dst, size_t &dstsz, void const *src, size_t srcsz);
};

#endif // COMPRESSION_HPP
