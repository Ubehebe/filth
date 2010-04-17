#ifndef GZIP_HPP
#define GZIP_HPP

#include <stdint.h>
#include <zlib.h>

/** \brief A thin wrapper on top of zlib for doing in-memory (de)compression.
 * This is a class instead of a namespace just in order to hide some stuff;
 * you can't actually make an instance of it.
 * \note The zlib documentation says it is thread-safe.
 * \note Usage of the terms "zlib", "gzip", and "deflate" can be confusing.
 * All of them can refer to formats of byte streams, while only the latter two
 * are valid HTTP/1.1 content-encoding headers. "zlib" is also the name of the
 * library and "deflate" is also the name of the algorithm used by the library.
 * The zlib FAQ (zlib.net/zlib_faq.html) is unequivocal: "Bottom line: use the
 * gzip format for HTTP 1.1 encoding." That is why it is the default in this
 * class. */
class gzip
{
public:
  enum format {
    GZIP, //!< "gzip" byte stream format
    ZLIB //!< "zlib" byte stream format
  };
  /** \brief Wrapper for zlib's compressBound.
   * Used before compression to get an upper bound on the amount of
   * space we need to allocate to store the compressed bytestream. */
  static size_t compressBound(size_t srcsz, format f=GZIP);
  /** \brief Wrapper for zlib's compress.
   * \param dst destination buffer
   * \param dstsz size of destination buffer (on return, contains actual size
   * of compressed stream)
   * \param src source buffer
   * \param srcsz size of source buffer
   * \param f output format. The default, gzip, is usually what you want.
   * \param level zlib compression level
   * \return true if compression succeeded, false else. */
  static bool compress(void *dst, size_t &dstsz, void const *src, size_t srcsz,
		       format f=GZIP, int level=Z_DEFAULT_COMPRESSION);
  /** \brief Wrapper to zlib's compress, but takes care of allocation.
   * \param src source buffer
   * \param srcsz size of source buffer
   * \param dstsz on return, contains size of compressed stream (0 on failure)
   * \param f output format. The default, gzip, is usually what you want.
   * \param level zlib compression level
   * \return pointer to newly allocated compressed buffer (NULL on failure) */
  static void *compress(void *src, size_t srcsz, size_t &dstsz,
			format f=GZIP, int level=Z_DEFAULT_COMPRESSION);
  /** \brief Wrapper to zlib's uncompress.
   * \param dst destination buffer
   * \param dstsz size of destination buffer (on return, contains actual size
   * of uncompressed stream)
   * \param src source buffer
   * \param srcsz size of source buffer
   * \param f expected input format. The default, gzip, is usually what you
   * are dealing with.
   * \return true on success, false on failure (for example, dst was too small)
   */
  static bool uncompress(void *dst, size_t &dstsz, void const *src,
			 size_t srcsz, format f=GZIP);
  static void *uncompress(size_t &resultsz, void const *src, size_t srcsz,
			  format f=GZIP, int start=2, int stop=5);
private:
  /* zlib's compressBound function gives you the upper bound for compressed
   * streams in zlib format, but the headers for the gzip format are a little
   * bigger. How much bigger? ...Uh...according to Wikipedia, 10 bytes for the
   * header, 8 bytes for the footer, and some "optional extra headers such as
   * the file name". I sure hope this is sufficient! I tested it on 1-byte-long files
   * (where an error would be most likely to show up, since "compressing"
   * actually increases the file size) and it seemed to work. */
  static size_t const gzip_hdr_extra = 18;
  gzip();
  static int README_compress2 (Bytef *dest, uLongf *destLen, const Bytef *source,
			       uLong sourceLen, int level);
  static int README_uncompress(Bytef *dest, uLongf *destLen,
			       const Bytef *source, uLong sourceLen);
};

#endif // GZIP_HPP
