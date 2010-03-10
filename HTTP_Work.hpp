#ifndef HTTP_WORK_HPP
#define HTTP_WORK_HPP

#include <iostream>
#include <map>
#include <sstream>
#include <stdint.h>
#include <string>
#include <sys/types.h>

#include "FileCache.hpp"
#include "LockedQueue.hpp"
#include "Scheduler.hpp"
#include "Work.hpp"

// Forward declaration for befriending HTTP_Work.
class HTTP_mkWork;

class HTTP_Work : public Work
{
  // Just so it can set the static members.
  friend class HTTP_mkWork;

  // Stuff that should be the same for all.
  static uint32_t const cbufsz = 1<<12;
  static LockedQueue<Work *> *q;
  static Scheduler *sch;
  static FileCache *cache;

  // Internal state.
  char cbuf[cbufsz];
  std::stringstream buf;
  bool endoflife;

  // No copying, no assigning.
  HTTP_Work(HTTP_Work const &);
  HTTP_Work &operator=(HTTP_Work const &);


  // Internal buffering.
  inline void get_from_client();
  inline void put_to_client();

  bool parse();
  void parse_req_line(std::string &line);
  void parse_header(std::string &line);
  void parse_uri(std::string &uri);
  // TODO: uri_hex_escape
  
public:
  void operator()();
  HTTP_Work(int fd, Work::mode m);
};

class HTTP_mkWork : public mkWork
{
public:
  Work *operator()(int fd, Work::mode m);
  /* Rationale: an HTTP_Work object is a particular kind of Work object:
   * it needs to know about the work queue and the scheduler, since it
   * might need to initiate new work. But the work queue and the scheduler
   * don't exist until the server constructor returns. Thus we should call init
   * in the body of the HTTP_Server constructor to set them. Note that this
   * means that we can't actually start the server from inside the server
   * constructor; we provide a serve() function instead. */
  void init(LockedQueue<Work *> *q, Scheduler *sch, FileCache *c);
};

#endif // HTTP_WORK_HPP
