#include "logging.h"
#include "MIME_FileCache.hpp"

Magic *MIME_FileCache::MIME_cinfo::lookup = NULL;

MIME_FileCache::MIME_cinfo::MIME_cinfo(std::string &path, size_t sz)
  : FileCache::cinfo(path, sz)
{
  MIME_type = strdup((*lookup)(path.c_str()));
}

MIME_FileCache::MIME_cinfo::~MIME_cinfo()
{
  free(const_cast<char *>(MIME_type));
}

FileCache::cinfo *MIME_FileCache::mkcinfo(std::string &path, size_t sz)
{
  return new MIME_cinfo(path, sz);
}

MIME_FileCache::MIME_FileCache(size_t max, FindWork &fwork)
  : FileCache(max, fwork)
{
  MIME_cinfo::lookup = &lookup;
}
