#ifndef HTTP_2616_SERVER_WORK_HPP
#define HTTP_2616_SERVER_WORK_HPP

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
#include "HTTP_Server_Work.hpp"
#include "Preallocated.hpp"
#include "Scheduler.hpp"
#include "Time_nr.hpp"

/** \brief Contains most of the protocol-specific logic of RFC 2616.
 * \todo lots
 */
class HTTP_2616_Server_Work
  : public HTTP_Server_Work, public Preallocated<HTTP_2616_Server_Work>
{
public:
  /** \param fd open connection
   * \param m should be Work::read */
  HTTP_2616_Server_Work(int fd, Work::mode m=Work::read);
  ~HTTP_2616_Server_Work();
  /** \brief The caching server should set this.
   * \param cache pointer to the cache. */
  static void setcache(Cache<std::string, HTTP_CacheEntry *> *cache);
  void async_setresponse(HTTP_Client_Work *assoc,
			 structured_hdrs_type const &resphdrs,
			 std::string const &respbody);
  
private:
  void browse_req(structured_hdrs_type &req_hdrs, std::string const &req_body);
  bool cache_get(std::string &path, HTTP_CacheEntry *&c);
  bool cache_put(std::string &path, HTTP_CacheEntry *c, size_t sz);
  void prepare_response(structured_hdrs_type &req_hdrs,
			std::string const &req_body,
			std::ostream &hdrstream,
			uint8_t const *&body,
			size_t &bodysz);
  void on_oops(HTTP_constants::status const &s, std::ostream &hdrstream);
  void reset();

  HTTP_2616_Server_Work(HTTP_2616_Server_Work const&);
  HTTP_2616_Server_Work &operator=(HTTP_2616_Server_Work const&);

  // State that is the same for all work objects.
  static Cache<std::string, HTTP_CacheEntry *> *cache;

  // Stuff reported by the client in the request headers.

  std::string path, query;
  HTTP_constants::method meth;

  /* Normally the response either comes straight from the cache or
   * comes from disk and is left in the cache for later use, but if neither of
   * these is true we need to remember to delete it ourselves. */
  bool resp_is_cached;
  HTTP_CacheEntry *c;
  uint8_t *uncompressed;


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
  HTTP_constants::content_coding cl_accept_enc, cl_content_enc;
  size_t cl_max_fwds;
};


#endif // HTTP_2616_SERVER_WORK_HPP
