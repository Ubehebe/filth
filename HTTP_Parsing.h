#ifndef HTTP_PARSING_H
#define HTTP_PARSING_H

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include "HTTP_constants.h"
#include "LockedQueue.h"
#include "Parsing.h"
#include "Scheduler.h"

// We have no reason to implement the client version yet.
class HTTP_Parsing : public ServerParsing
{
  bool req_line_done;
  std::string resource_str;
  // The stuff after the "?" in a URI.
  std::string query;

  HTTP_constants::method meth;
  HTTP_constants::status stat;

  /* Pointers to stuff defined elsewhere. These are static because they
   * should be the same for all instances; they should be set once and for
   * all by the server's HTTP_Parsing object.
   *
   * The reason the parsing objects need all this global data is because
   * they may need to initiate a connection (a Unix domain socket) 
   * if they find out the resource is dynamic. */
  static std::map<std::string, std::pair<time_t, std::string *> > *cache;
  static std::map<int, std::pair<std::stringstream *, Parsing *> > *state;
  static LockedQueue<std::pair<int, char> > *q;
  static Scheduler *sch;

  void parse_req_line(std::string &line);
  void parse_header(std::string &line);
  void parse_uri(std::string &uri);
  // TODO: uri_hex_escape!!!
public:
  std::string response_body;
  std::ostream &put_to(std::ostream &o);
  std::istream &get_from(std::istream &i);
  HTTP_Parsing(std::map<std::string, std::pair<time_t, std::string *> > 
	       *_cache,
	       std::map<int, std::pair<std::stringstream *, Parsing *> >
	       *_state,
	       LockedQueue<std::pair<int, char> > *_q, Scheduler *_sch)
    : req_line_done(false)
  {
    cache = _cache;
    state = _state;
    q = _q;
    sch = _sch;
  }
  HTTP_Parsing() : req_line_done(false) {}
  Parsing *mkParsing();
};

#endif // HTTP_PARSING_H
