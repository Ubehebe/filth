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
 prepare_startover:

  HTTP_CacheEntry *c = NULL;
  int err;

  // Ask the cache if we have a response ready to go.
  if (!cl_cache_control.isset(cache_control::no_cache)
      && cache->get(path, c)) {
    bool warn_stale = false;

    /* 14.9.3: min-fresh "Indicates that the client is willing to accept a
     * response whose freshness lifetime is no less than its current age
     * plus the specified time in seconds". */
    if (c->response_is_fresh() || HTTP_Origin_Server::validate(path, c)) {
      if (!cl_cache_control.isset(cache_control::use_min_fresh)
	  || c->freshness_lifetime() >=
	  c->current_age() + cl_cache_control.min_fresh)
	goto cache_ok;
      hdrs << *c; // Pushes the status line, then all the headers
      _LOG_DEBUG("%s", hdrs.str().c_str());
      body = c->getbuf();
      bodysz = const_cast<size_t &>(c->szincache);
    }
    else {
      // This can't actually evict it until we call unget (in the destructor).
      cache->advise_evict(path);
    
      /* 14.9.3: max-age "Indicates that the client is willing to accept a
       * response whose age is no greater than the specified time in seconds.
       * Unless max-stale directive is also included, the client is not willing
       * to accept a stale response."
       *
       * "If both the new request and the cached entry include "max-age"
       * directives, then the lesser of the two values is used for determining
       * the freshness of the cached entry for that request." Man! */
      if (cl_cache_control.isset(cache_control::use_max_age)) {
	time_t lesser = cl_cache_control.max_age;
	if (c->use_max_age && c->max_age_value < lesser)
	  lesser = c->max_age_value;
	if (c->current_age() <= lesser && (c->response_is_fresh()
					   || (cl_cache_control.isset(cache_control::use_max_stale)
					       && c->current_age() - c->freshness_lifetime()
					       <= cl_cache_control.max_stale)))
	  goto cache_ok;
	else
	  goto cache_no;
      }

      /* 14.9.3: max-stale "Indicates that the client is willing to accept a
       * response that has exceeded its expiration time. If max-stale is
       * assigned a value, then the client is willing to accept a response that
       * has exceeded its expiration time by no more than the specficied number
       * of seconds. If no value is assigned to max_stale, then the client is
       * willing to acccept a stale response of any age." */
      if (cl_cache_control.isset(cache_control::use_max_stale)) {
	if (c->current_age() - c->freshness_lifetime()
	    <= cl_cache_control.max_stale)
	  goto cache_ok;
	else
	  goto cache_no;
      }
      else {
	goto prepare_startover; // ???
      }
    }
 cache_ok:
 cache_no:
    ;// TODO: cache is byzantine!
  }

  // If it's not in the cache, ask the origin server.
  else if ((err = HTTP_Origin_Server::request(path, c))==0) {
    /* Any header that requires thread-safe state to construct (like
     * the MIME lookup) gets pushed here. */
    c->pushstat(stat);
    c->pushhdr(Content_Type, (*MIME)(path.c_str()));
    c->pushhdr(Date, (*date)());
    c->pushhdr(Last_Modified, (*date)(c->last_modified));
    // Might not succeed...
    cache->put(path, c, c->szincache);
    hdrs << *c; // Pushes the status line, then all the headers
    _LOG_DEBUG("%s", hdrs.str().c_str());
    body = c->getbuf();
    bodysz = const_cast<size_t &>(c->szincache);
  }
  
  else {
    // Make more elaborate?
    switch (err) {
    case ENOENT:
      throw HTTP_Parse_Err(Not_Found);
    case EACCES:
      throw HTTP_Parse_Err(Forbidden);
    default:
      throw HTTP_Parse_Err(Internal_Server_Error);
    }
  }
}

void HTTP_Server_Work::on_parse_err(status &s, stringstream &hdrs,
				    uint8_t const *&body, size_t &bodysz)
{
  hdrs << HTTP_Version << ' ' << s << CRLF
       << Date << (*date)() << CRLF
       << Server << PACKAGE_NAME << CRLF
       << Content_Length << 0 << CRLF
       << CRLF;
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
      cl_cache_control.set(cache_control::no_cache);
    else if (tmp.find("no-store") != tmp.npos)
      cl_cache_control.set(cache_control::no_store);
    else if (tmp.find("no-transform") != tmp.npos)
      cl_cache_control.set(cache_control::no_transform);
    else if (tmp.find("only-if-cached") != tmp.npos)
      cl_cache_control.set(cache_control::only_if_cached);
    else if (tmp.find("max-age") != tmp.npos) {
      cl_cache_control.set(cache_control::use_max_age);      
      stringstream tmp2(tmp);
      tmp2 >> tmp; // gets rid of max-age
      tmp2 >> tmp; // gets rid of =
      tmp2 >> cl_cache_control.max_age;
    }
    else if (tmp.find("max-stale") != tmp.npos) {
      cl_cache_control.set(cache_control::use_max_stale);
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
      cl_cache_control.set(cache_control::use_min_fresh);      
      stringstream tmp2(tmp);
      tmp2 >> tmp; // gets rid of min-fresh
      tmp2 >> tmp; // gets rid of =
      tmp2 >> cl_cache_control.min_fresh;
    }

    /* 14.9.1: "If the no-cache directive does not specify a field-name"
     * (which it cannot in a cache-request-directive) "then a cache MUST NOT
     * use the response to satisfy a subsequent response without successful
     * revalidation with the origin server".
     *
     * 14.9.2: If no-store is sent in a request, "a cache MUST NOT store any
     * part of either this request or any response to it". */
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
