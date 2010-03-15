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
  friend class HTTP_FindWork;
  // No copying, no assigning.
  HTTP_Work(HTTP_Work const&);
  HTTP_Work &operator=(HTTP_Work const&);

  static size_t const rdbufsz = 1<<10;
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
  char *outgoing_offset;
  
  /* The main internal driver functions to get stuff from the client and
   * return stuff to the client. */
  void incoming();
  void outgoing();
  void outgoing(size_t &towrite);

  void format_status_line();
  bool parse();
  void parse_req_line(std::string &line);
  void parse_header(std::string &line);
  void parse_uri(std::string &uri);
  // TODO: uri_hex_escape
  
public:
  void operator()();
  HTTP_Work(int fd, Work::mode m);
  ~HTTP_Work();
};

#endif // HTTP_WORK_HPP
