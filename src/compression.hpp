#ifndef COMPRESSION_HPP
#define COMPRESSION_HPP

#include <stdint.h>
#include <zlib.h>

/* \brief A thin wrapper on top of zlib.
 * \note The zlib documentation says it is thread-safe.
 * \todo Support other forms of compression: gzip, compress. */
namespace compression
{
  /** \brief Wrapper for zlib's compressBound.
   * Used before compression to get an upper bound on the amount of
   * space we need to allocate to store the compressed bytestream. */
  size_t compressBound(size_t srcsz);
  /** \brief Wrapper for zlib's compress.
   * \param dst destination buffer
   * \param dstsz size of destination buffer (on return, contains actual size
   * of compressed stream)
   * \param src source buffer
   * \param srcsz size of source buffer
   * \param level zlib compression level
   * \return true if compression succeeded, false else. */
  bool compress(void *dst, size_t &dstsz, void const *src, size_t srcsz,
		int level=Z_DEFAULT_COMPRESSION);
  /** \brief Wrapper to zlib's compress, but takes care of allocation.
   * \param src source buffer
   * \param srcsz size of source buffer
   * \param dstsz on return, contains size of compressed stream (0 on failure)
   * \param level zlib compression level
   * \return pointer to newly allocated compressed buffer (NULL on failure) */
  void *compress(void const *src, size_t srcsz, size_t &dstsz,
		 int level=Z_DEFAULT_COMPRESSION);
  /** \brief Wrapper to zlib's uncompress.
   * \param dst destination buffer
   * \param dstsz size of destination buffer (on return, contains actual size
   * of uncompressed stream)
   * \param src source buffer
   * \param srcsz size of source buffer
   * \return true on success, false on failure (for example, dst was too small)
   */
  bool uncompress(void *dst, size_t &dstsz, void const *src, size_t srcsz);
};

#endif // COMPRESSION_HPP
