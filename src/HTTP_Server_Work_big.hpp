#ifndef HTTP_SERVER_WORK_BIG_HPP
#define HTTP_SERVER_WORK_BIG_HPP

#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <unordered_map>

#include "FindWork_prealloc.hpp"
#include "HTTP_constants.hpp"
#include "LockFreeQueue.hpp"
#include "HTTP_Cache.hpp"
#include "HTTP_Client_Work_Unix.hpp"
#include "HTTP_Server_Work.hpp"
#include "Magic_nr.hpp"
#include "Preallocated.hpp"
#include "Scheduler.hpp"
#include "Time_nr.hpp"

/* HTTP_Server_Work is a minimal class for work that typically gets handled
 * by servers that speak HTTP; this class is "big" in that its behavior tries to
 * hew to RFC 2616. */
class HTTP_Server_Work_big
  : public HTTP_Server_Work, public Preallocated<HTTP_Server_Work_big>
{
public:
  HTTP_Server_Work_big(int fd, Work::mode m=Work::read);
  ~HTTP_Server_Work_big();
  
private:
  void browse_req(structured_hdrs_type &req_hdrs, std::string const &req_body);
  bool cache_get(std::string &path, HTTP_CacheEntry *&c);
  bool cache_put(std::string &path, HTTP_CacheEntry *c, size_t sz);
  void prepare_response(structured_hdrs_type &req_hdrs,
			std::string const &req_body,
			std::ostream &hdrstream,
			uint8_t const *&body,
			size_t &bodysz);
  void on_parse_err(HTTP_constants::status &s, std::ostream &hdrstream,
		    uint8_t const *&body, size_t &bodysz);
  void reset();

  // So it can set wmap
  friend class FindWork_prealloc<HTTP_Server_Work_big>;
  //  friend class HTTP_FindWork;
  friend class HTTP_Worker;
  HTTP_Server_Work_big(HTTP_Server_Work_big const&);
  HTTP_Server_Work_big &operator=(HTTP_Server_Work_big const&);

  // State that is the same for all work objects.
  static HTTP_Cache *cache;
  static FindWork_prealloc<HTTP_Server_Work_big>::workmap *wmap;

  // State imbued from the Worker object that we use.
  Time_nr *date;
  Magic_nr *MIME;

  // Stuff reported by the client in the request headers.

  std::string path, query;
  HTTP_constants::method meth;

  /* Normally the response either comes straight from the cache or
   * comes from disk and is left in the cache for later use, but if neither of
   * these is true we need to remember to delete it ourselves. */
  bool resp_is_cached;
  HTTP_CacheEntry *c;

  HTTP_Client_Work_Unix *dynamic_resource; // yikes
  

  struct cc
  {
    enum opts {
      no_cache = 1<<0,
      no_store = 1<<1,
      no_transform = 1<<2,
      only_if_cached = 1<<3,
      use_max_age = 1<<4,
      use_min_fresh = 1<<5,
      use_max_stale = 1<<6 };
    uint8_t flags; // Remember to change if more than 8 flags :)
    time_t max_age, min_fresh, max_stale;
    cc() : flags(0), max_age(0), min_fresh(0), max_stale(0) {}
    inline void set(opts o) { flags |= o; }
    inline bool isset(opts o) { return flags & o; }
    inline void clear() { flags = 0; }
  };
  cc cl_cache_control;
  HTTP_constants::content_coding cl_accept_enc;
  size_t cl_max_fwds;
};


#endif // HTTP_SERVER_WORK_BIG_HPP
