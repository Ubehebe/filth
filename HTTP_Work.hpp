#ifndef HTTP_WORK_HPP
#define HTTP_WORK_HPP

#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <unordered_map>

#include "FileCache.hpp"
#include "HTTP_constants.hpp"
#include "HTTP_Statemap.hpp"
#include "LockedQueue.hpp"
#include "Scheduler.hpp"
#include "Work.hpp"

class HTTP_Work : public Work
{
  // Stuff that should be the same for all.
  static size_t const rdbufsz = 1<<10;
  static LockedQueue<Work *> *q;
  static Scheduler *sch;
  static FileCache *cache;
  static HTTP_Statemap *st;

  // Internal state.
  char rdbuf[rdbufsz]; // Buffer to use to read from client...and response??
  std::string path; // Path to resource
  std::string query; // The stuff after the "?" in a URI; to pass to resource
  char *resource; // Raw pointer to resource contents
  std::stringstream pbuf; // Buffer to use in parsing
  HTTP_constants::status stat; // Status code we'll return to client
  HTTP_constants::method meth; // Method (GET, POST, etc.)
  size_t resourcesz; // Size of resource (for static only??)
  size_t statlnsz; // Size of status line
  bool req_line_done, status_line_done;
  
  // No copying, no assigning.
  HTTP_Work(HTTP_Work const &);
  HTTP_Work &operator=(HTTP_Work const &);


  /* The main internal driver functions to get stuff from the client and
   * return stuff to the client. */
  inline void incoming();
  inline void outgoing();
  inline void outgoing(char **buf, size_t *towrite);

  inline void format_status_line();
  bool parse();
  void parse_req_line(std::string &line);
  void parse_header(std::string &line);
  void parse_uri(std::string &uri);
  // TODO: uri_hex_escape
  
public:
  void operator()();
  Work *getwork(int fd, Work::mode m);
  HTTP_Work();
  HTTP_Work(int fd, Work::mode m);
  HTTP_Work(LockedQueue<Work *> *q, Scheduler *sch, FileCache *c,
	    HTTP_Statemap *st);
  ~HTTP_Work();
};


/* Rationale: an HTTP_Work object is a particular kind of Work object:
 * it needs to know about the work queue and the scheduler, since it
 * might need to initiate new work. But the work queue and the scheduler
 * don't exist until the server constructor returns. Thus we should call init
 * in the body of the HTTP_Server constructor to set them. Note that this
 * means that we can't actually start the server from inside the server
 * constructor; we provide a serve() function instead. */

#endif // HTTP_WORK_HPP
