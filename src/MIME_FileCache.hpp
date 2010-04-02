#ifndef MIME_FILECACHE_HPP
#define MIME_FILECACHE_HPP

#include <string.h>

#include "FileCache.hpp"
#include "Magic.hpp"

class MIME_FileCache : public FileCache
{
  MIME_FileCache(MIME_FileCache const &);
  MIME_FileCache &operator=(MIME_FileCache const &);
  Magic lookup;
  virtual FileCache::cinfo *mkcinfo(std::string &path, size_t sz);
public:
  class MIME_cinfo : public FileCache::cinfo
  {
    friend class MIME_FileCache;
    static Magic *lookup;
  public:
    char const *MIME_type;
    MIME_cinfo(std::string &path, size_t sz);
    ~MIME_cinfo();
  };
  MIME_FileCache(size_t max, FindWork &fwork);
  ~MIME_FileCache() {}
};

#endif // MIME_FILECACHE_HPP
