#include <algorithm>
#include <errno.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "compression.hpp"
#include "config.h" // For PACKAGE_NAME
#include "HTTP_cmdline.hpp"
#include "HTTP_Origin_Server.hpp"
#include "HTTP_Parse_Err.hpp"
#include "HTTP_Server_Work.hpp"
#include "logging.h"
#include "ServerErrs.hpp"
#include "util.hpp"

using namespace std;
using namespace HTTP_constants;

LockFreeQueue<void *> HTTP_Server_Work::store;
HTTP_Cache *HTTP_Server_Work::cache = NULL;
Workmap *HTTP_Server_Work::st = NULL;
Time *HTTP_Server_Work::date = NULL;
Magic *HTTP_Server_Work::MIME = NULL;

HTTP_Server_Work::HTTP_Server_Work(int fd, Work::mode m)
  : HTTP_Work(fd, m), cl_accept_enc(HTTP_constants::identity)
{
}

HTTP_Server_Work::~HTTP_Server_Work()
{
  cache->unget(path); // fails if path isn't in cache, which is fine
  st->erase(fd);
}


/* After parsing is complete, we talk to the cache and the origin server,
 * if need be, to find the resource. */
void HTTP_Server_Work::prepare_response(stringstream &hdrs,
					uint8_t const *&body, size_t &bodysz)
{
  HTTP_CacheEntry *c = NULL;
  int err;

  /* First ask the cache if we have a ready response, taking into account
   * the client's caching preferences. */
  if (consult_cache(path, c)) {
    _LOG_DEBUG("using cache to service %s", path.c_str());
    ; // fall through to success
  }

  /* If the only-if-cache directive is set and we didn't find it in the cache,
   * that's failure. According to 14.9.3, the correct status is, for some
   * reason, 504 Gateway Timeout. */
  else if (cl_cache_control.isset(cc::only_if_cached)) {
    throw HTTP_Parse_Err(Gateway_Timeout);
  }

  /* Ask the origin server. */
  else if ((err = HTTP_Origin_Server::request(path, c))==0) {
    /* Any header that requires thread-safe state to construct (like
     * the MIME lookup) gets pushed here. */
    c->pushstat(stat);
    c->pushhdr(Content_Type, (*MIME)(path.c_str()));
    c->pushhdr(Date, (*date)());
    c->pushhdr(Last_Modified, (*date)(c->last_modified));

    /* 14.9.2: If no-store is sent in a request, "a cache MUST NOT store
     * any part of either this request or any response to it."
     * Note that this seems to include any sort of header logging
     * (user-agent, etc.) */
    if (!cl_cache_control.isset(cc::no_store) &&
	!cache->put(path, c, c->szincache))
      _LOG_INFO("wanted to cache %s, but was unsuccessful", path.c_str());
  }
  else if (err == ENOENT) {
    throw HTTP_Parse_Err(Not_Found);
  }
  else if (err == EACCES) {
    throw HTTP_Parse_Err(Forbidden);
  }
  /*  else if (err == EISDIR)
    // directory algorithm!
  else if (err == EISPIPE)
    // dynamic resource algorithm!
    */
  // General-purpose error.
  else {
    throw HTTP_Parse_Err(Internal_Server_Error);
  }

  // If we're here, c contains some kind of response.
  hdrs << *c; // Pushes the status line, then all the headers
  _LOG_DEBUG("%s", hdrs.str().c_str());
  body = c->getbuf();
  bodysz = const_cast<size_t &>(c->szincache);
}

/* Tries to make sense of RFC 2616, secs. 13 and 14.9. It's probably not
 * 100% right though. */
bool HTTP_Server_Work::consult_cache(string &path, HTTP_CacheEntry *&c)
{
  /* I am still getting valgrind "uninitialized value" errors here,
   * though not regularly, and no segfaults. */


  /* 14.9.4: "The request includes a "no-cache" cache-control directive" ...
   * "The server MUST NOT use a cached copy when responding to such
   * a request." */
  if (cl_cache_control.isset(cc::no_cache) || !cache->get(path,c))
    return false;

  bool fresh = c->response_is_fresh();
  bool valid = HTTP_Origin_Server::validate(path, c);

  /* For use at cache_ok. I hate that C/C++ doesn't let you begin a label
   * with a declaration. */
  stringstream tmp;
  string age;

  /* As far as I understand, the only cache directive that can disqualify
   * a fresh or valid cache entry (other than no-cache) is min-fresh. */
  if ((fresh || valid) && !cl_cache_control.isset(cc::use_min_fresh))
    goto cache_ok;

  /* 14.9.3: min-fresh "Indicates that the client is willing to accept a
   * response whose freshness lifetime is no less than its current age
   * plus the specified time in seconds". */
  if (cl_cache_control.isset(cc::use_min_fresh)) {
    if (c->freshness_lifetime() >=
	c->current_age() + cl_cache_control.min_fresh)
      goto cache_ok;
    else
      goto cache_no;
  }
    
  /* 14.9.3: max-age "Indicates that the client is willing to accept a
   * response whose age is no greater than the specified time in seconds.
   * Unless max-stale directive is also included, the client is not willing
   * to accept a stale response."
   *
   * "If both the new request and the cached entry include "max-age"
   * directives, then the lesser of the two values is used for determining
   * the freshness of the cached entry for that request." Man! */
  if (cl_cache_control.isset(cc::use_max_age)) {
	time_t lesser = (c->use_max_age && c->max_age_value < lesser)
	  ? c->max_age_value
	  : cl_cache_control.max_age;
	if (c->current_age() <= lesser) {
	  if (cl_cache_control.isset(cc::use_max_stale)) {
	    if (c->current_age() - c->freshness_lifetime()
		<= cl_cache_control.max_stale) {
	      goto cache_ok;
	    } else {
	      goto cache_no;
	    }
	  } else {
	    goto cache_ok;
	  }
	} else {
	  goto cache_no;
	}
  }

  /* 14.9.3: max-stale "Indicates that the client is willing to accept a
   * response that has exceeded its expiration time. If max-stale is
   * assigned a value, then the client is willing to accept a response that
   * has exceeded its expiration time by no more than the specficied number
   * of seconds. If no value is assigned to max_stale, then the client is
   * willing to acccept a stale response of any age." */
  if (cl_cache_control.isset(cc::use_max_stale)) {
    if (c->current_age() - c->freshness_lifetime()
	<= cl_cache_control.max_stale)
      goto cache_ok;
    else
      goto cache_no;
  }

 cache_ok:
  /* 13.2.3: "When a response is generated from a cache entry, the cache
   * MUST include a single Age header field in the response with a value
   * equal to the cache entry's current_age." */
  tmp << c->current_age();
  tmp >> age;
  c->pushhdr(Age, age);
  if (!fresh) {
    /* Even if we're using a stale entry, we should probably tell the cache to
     * get rid of it when it can. This is my hunch, nothing official. */
    cache->advise_evict(path);
    c->pushhdr(Warning, "110 " PACKAGE_NAME " \"Response is stale\"");
  }
  return true;

 cache_no:
  cache->unget(path);
  return false;
}

void HTTP_Server_Work::on_parse_err(status &s, stringstream &hdrs,
				    uint8_t const *&body, size_t &bodysz)
{
  
  hdrs << HTTP_Version << ' ' << s << CRLF
       << Date << (*date)() << CRLF
       << Server << PACKAGE_NAME << CRLF
       << Content_Length << 0 << CRLF
       << CRLF;
  _LOG_DEBUG("%s", hdrs.str().c_str());
  body = NULL;
  bodysz = 0;
}

// Comments in this block refer to sections of RFC 2616.
void HTTP_Server_Work::browse_req(HTTP_Work::req_hdrs_type &req_hdrs,
				  string const &req_body)
{
  parsereqln(req_hdrs, meth, path, query);
  if (path == "/")
    path = HTTP_cmdline::c.svals[HTTP_cmdline::default_resource];

  if (!req_hdrs[Accept_Encoding].empty()) {
    /* 14.3: "If an Accept-Encoding field is present in a request, and if the
     * server cannot send a response which is acceptable to the Accept-Encoding
     * header, then the server SHOULD send an error response with the 406
     * (Not Acceptable) status code."
     *
     * According to 3.5, the content codings are case insensitive. */
    
    string &tmp = util::tolower(req_hdrs[Accept_Encoding]);
    if (tmp.find("deflate") != tmp.npos)
      cl_accept_enc = HTTP_constants::deflate;
    else if (tmp.find("identity") != tmp.npos)
      cl_accept_enc = HTTP_constants::identity;
    else throw HTTP_Parse_Err(Not_Acceptable);
  }

  if (!req_hdrs[Cache_Control].empty()) {
    /* 14.9: Cache-Control = "Cache-Control" ":" 1#cache-directive
     * cache-directive = cache-request-directive | cache-response-directive
     * cache-request-directive = 
     * "no-cache"
     * | "no-store"
     * | "max-age" "=" delta-seconds
     * | "max-stale" [ "=" delta-seconds]
     * | "min-fresh" "=" delta-seconds
     * | "no-transform"
     * | "only-if-cached"
     * | cache-extension
     */
    string &tmp = req_hdrs[Cache_Control];
    if (tmp.find("no-cache") != tmp.npos)
      cl_cache_control.set(cc::no_cache);
    else if (tmp.find("no-store") != tmp.npos)
      cl_cache_control.set(cc::no_store);
    else if (tmp.find("no-transform") != tmp.npos)
      cl_cache_control.set(cc::no_transform);
    else if (tmp.find("only-if-cached") != tmp.npos)
      cl_cache_control.set(cc::only_if_cached);
    else if (tmp.find("max-age") != tmp.npos) {
      cl_cache_control.set(cc::use_max_age);      
      stringstream tmp2(tmp);
      tmp2 >> tmp; // gets rid of max-age
      tmp2 >> tmp; // gets rid of =
      tmp2 >> cl_cache_control.max_age;
    }
    else if (tmp.find("max-stale") != tmp.npos) {
      cl_cache_control.set(cc::use_max_stale);
      if (tmp.find("=") != tmp.npos) {
	stringstream tmp2(tmp);
	tmp2 >> tmp; // gets rid of max-stale
	tmp2 >> tmp; // gets rid of =
	tmp2 >> cl_cache_control.max_stale;
      } else {
	// Hopefully becomes a large positive value
	cl_cache_control.max_stale = -1;
      }
    }
    else if (tmp.find("min-fresh") != tmp.npos) {
      cl_cache_control.set(cc::use_min_fresh);      
      stringstream tmp2(tmp);
      tmp2 >> tmp; // gets rid of min-fresh
      tmp2 >> tmp; // gets rid of =
      tmp2 >> cl_cache_control.min_fresh;
    }


  }

  if (!req_hdrs[Connection].empty()) {
    /* 14.10: "HTTP/1.1 defines the "close" connection option for the sender to
     * signal that the connection will be closed after completion of the
     * response." */
    if (req_hdrs[Connection].find("close") != string::npos)
      closeme = true;
  }

  if (!req_hdrs[Expect].empty()) {
    /* 14.20: Expect = "Expect" ":" 1#expectation
     * expectation = "100-continue" | expectation-extension
     *
     * "A server that does not understand or is unable to comply with any of the
     * expectation values in the Expect field of a request MUST respond with
     * appropriate error status. The server MUST respond with a 417
     * (Expectation Failed) status if any of the expectations cannot be met or,
     * if there are other problems with the request, some other 4xx status."
     * Also: "Comparison of expectation values is case-insensitive for
     * unquoted strings (including the 100-continue token)".
     * 
     * The behavior of 100-continue is governed by 8.2.3. */
    if (strcasecmp("100-continue", req_hdrs[Expect].c_str())==0)
      throw HTTP_Parse_Err(Continue);
    else
      throw HTTP_Parse_Err(Expectation_Failed);
  }
  if (!req_hdrs[Max_Forwards].empty()) {
    stringstream tmp(req_hdrs[Max_Forwards]);
    tmp >> cl_max_fwds;
  }
}

void *HTTP_Server_Work::operator new(size_t sz)
{
  void *stuff;
  if (!store.nowait_deq(stuff)) {
    stuff = ::operator new(sz);
  }
  return stuff;
}

void HTTP_Server_Work::operator delete(void *work)
{
  store.enq(work);
}

void HTTP_Server_Work::reset()
{
  path.clear();
  query.clear();
  cl_cache_control.clear();
  // What about cl_accept_enc and cl_max_fwds, meth, etc.
}
