#include "compression.hpp"

namespace compression
{
  size_t compressBound(size_t srcsz) { return ::compressBound(srcsz); }
  bool compress(void *dst, size_t &dstsz, void const *src, size_t srcsz,
		int level)
  {
    return (::compress2(reinterpret_cast<Bytef *>(dst),
			reinterpret_cast<uLongf *>(&dstsz),
			reinterpret_cast<Bytef const *>(src), srcsz, level) == Z_OK);
  }
  void *compress(void const *src, size_t srcsz, size_t &dstsz, int level)
  {
    dstsz = ::compressBound(srcsz);
    uint8_t *dst = new uint8_t[dstsz];
    if (compress(reinterpret_cast<void *>(dst), dstsz, src, srcsz, level)) {
      return reinterpret_cast<void *>(dst);
    } else {
      delete[] dst;
      dstsz = 0;
      return NULL;
    }
  }
  bool uncompress(void *dst, size_t &dstsz, void const *src, size_t srcsz)
  {
    return (::uncompress(reinterpret_cast<Bytef *>(dst),
			 reinterpret_cast<uLongf *>(&dstsz),
			 reinterpret_cast<Bytef const *>(src), srcsz) == Z_OK);
  }
};
