#include <iostream> // just for bad_alloc

#include "gzip.hpp"
#include "logging.h"

size_t gzip::compressBound(size_t srcsz, gzip::format f)
{ 
  size_t ans = ::compressBound(srcsz);
  if (f == gzip::GZIP)
    ans += gzip_hdr_extra;
  return ans;
}

bool gzip::compress(void *dst, size_t &dstsz, void const *src, size_t srcsz,
		    gzip::format f, int level)
{
  return (f == gzip::GZIP) 
    ? (README_compress2(reinterpret_cast<Bytef  *>(dst),
			reinterpret_cast<uLongf *>(&dstsz),
			reinterpret_cast<Bytef const *>(src), srcsz, level) == Z_OK)
    : (::compress2(reinterpret_cast<Bytef *>(dst),
		   reinterpret_cast<uLongf *>(&dstsz),
		   reinterpret_cast<Bytef const *>(src), srcsz, level) == Z_OK);

}

void *gzip::compress(void *src, size_t srcsz, size_t &dstsz,
		     gzip::format f, int level)
{
  dstsz = ::compressBound(srcsz);
  uint8_t *dst = new uint8_t[dstsz];
  if (compress(reinterpret_cast<void *>(dst), dstsz, src, srcsz, f, level)) {
    return reinterpret_cast<void *>(dst);
  } else {
    delete[] dst;
    dstsz = 0;
    return NULL;
  }
}

void *gzip::uncompress(size_t &resultsz, void const *src, size_t srcsz, format f,
		       int start, int stop)
{
  char *tmp;
  for (int multiplier = start; multiplier < stop; ++multiplier) {
    resultsz = multiplier * srcsz;
    try {
      tmp = new char[resultsz];
    } catch (std::bad_alloc) {
      resultsz = 0;
      return NULL;
    }
    if (uncompress(reinterpret_cast<void *>(tmp), resultsz, src, srcsz, f))
      break;
    delete tmp;
    tmp = NULL;
    resultsz = 0;
  }
  return reinterpret_cast<void *>(tmp);
}

bool gzip::uncompress(void *dst, size_t &dstsz, void const *src, size_t srcsz,
		      gzip::format f)
{
  return (f == gzip::GZIP) 
    ? (README_uncompress(reinterpret_cast<Bytef *>(dst),
			 reinterpret_cast<uLongf *>(&dstsz),
			 reinterpret_cast<Bytef const *>(src), srcsz) == Z_OK)
    : (::uncompress(reinterpret_cast<Bytef *>(dst),
		    reinterpret_cast<uLongf *>(&dstsz),
		    reinterpret_cast<Bytef const *>(src), srcsz) == Z_OK);
}


/* This function has been copied and pasted directly from the compress2.c
 * source file of zlib, with the single change indicated to switch from the zlib
 * format to the gzip format. */
int gzip::README_compress2 (Bytef *dest, uLongf *destLen,
				   const Bytef *source, uLong sourceLen, int level)
{
  z_stream stream;
  int err;
  
  stream.next_in = (Bytef*)source;
  stream.avail_in = (uInt)sourceLen;
#ifdef MAXSEG_64K
  /* Check for source > 64K on 16-bit machine: */
  if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;
#endif
  stream.next_out = dest;
  stream.avail_out = (uInt)*destLen;
  if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;
  
  stream.zalloc = (alloc_func)0;
  stream.zfree = (free_func)0;
  stream.opaque = (voidpf)0;
  
  // CHANGED
  //   err = deflateInit(&stream, level);
  err = deflateInit2(&stream, level, Z_DEFLATED, 24, 8, Z_DEFAULT_STRATEGY);
  if (err != Z_OK) return err;

  err = deflate(&stream, Z_FINISH);
  if (err != Z_STREAM_END) {
    deflateEnd(&stream);
    return err == Z_OK ? Z_BUF_ERROR : err;
  }
  *destLen = stream.total_out;

  err = deflateEnd(&stream);
  return err;
}

/* This function has been copied and pasted directly from the uncompr.c
 * source file of zlib, with the single change indicated to switch from the zlib
 * format to the gzip format. */
int gzip::README_uncompress(Bytef *dest, uLongf *destLen,
				   const Bytef *source, uLong sourceLen)
{
    z_stream stream;
    int err;

    stream.next_in = (Bytef*)source;
    stream.avail_in = (uInt)sourceLen;
    /* Check for source > 64K on 16-bit machine: */
    if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;

    stream.next_out = dest;
    stream.avail_out = (uInt)*destLen;
    if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;

    // CHANGED
    // err = inflateInit(&stream);
    err = inflateInit2(&stream, 40);
    if (err != Z_OK) return err;

    err = inflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        inflateEnd(&stream);
        if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
            return Z_DATA_ERROR;
        return err;
    }
    *destLen = stream.total_out;

    err = inflateEnd(&stream);
    return err;
}
