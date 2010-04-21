#ifndef MAGIC_NR_HPP
#define MAGIC_NR_HPP

#include <magic.h>
#include <stdlib.h>

#include "logging.h"

/** \brief Thin wrapper around libmagic.
 * \warning The libmagic documentation I have seen does not mention whether
 * it is thread-safe, so I am assuming it is not ("_nr" means "non-reentrant").
 * Besides, it probably makes sense for each worker to have one of these.
 * \warning I am not using this class right now, because I have discovered that
 * libmagic is not all that reliable. I have seen it give CSS files a MIME type of
 * text/x-c, for example, which causes some browsers not to load the style
 * sheet, a major problem. Servers are probably better off with the mime_types
 * namespace, which takes a different approach, assigning MIME types based
 * on filename suffixes, and only using this class as a fallback, if at all. */
class Magic_nr
{
  magic_t m;
  static size_t const bufsz = 20;
  char buf[bufsz];
public:
  /** \brief Wrapper for magic_open and magic_load. */
  Magic_nr(int flags=MAGIC_MIME_TYPE, char const *database=NULL)
  {
    if ((m = magic_open(flags))==NULL) {
      _LOG_FATAL("magic_open: %m");
      exit(1);
    }
    if (magic_load(m, database)==-1) {
      _LOG_FATAL("magic_load: %m");
      exit(1);
    }
  }
  /** \brief Wrapper for magic_close. */
  ~Magic_nr() { magic_close(m); }
  /** \brief Wrapper for magic_file. */
  char const *operator()(char const *filename)
  {
    // Oh my goodness! This is turning out to be more consistent than libmagic.
    char const *suffix = strrchr(filename, '.');
    snprintf(buf, bufsz, "text/%s", (suffix == NULL) ? "plain" : suffix+1);
    _LOG_DEBUG("%s", buf);
    return buf;
    
    
    /*    char const *provisional = magic_file(m, filename);
    _LOG_DEBUG("%s: %s", filename, provisional);
    return (strncmp(provisional, "text/x-c", strlen("text/x-c"))==0)
    ? "text/css" : provisional; */
  }
  /** \brief Wrapper for magic_setflags. */
  void setflags(int flags)
  {
    if (magic_setflags(m, flags)==-1) {
      _LOG_FATAL("magic_setflags: %m");
      exit(1);
    }
  }
};

#endif // MAGIC_NR_HPP
