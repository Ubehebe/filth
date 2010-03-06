#ifndef PARSING_H
#define PARSING_H

#include <iostream>
#include <stdint.h>

class Parsing
{
  enum _flags
    {
      _closeonread = 1<<0, // Should worker close when done reading?
      _closeonwrite = 1<<1, // Should worker close when done writing?
      _parse_complete = 1<<2, // Has parsing completed?
      _sched_counterop = 1<<3, // See below
    };
  /* _sched_counterop:
   * Normally when a worker is conducting a read or write on behalf of
   * a parsing object and that operation completes, the worker will 
   * automatically schedule the complementary operation on the same file 
   * descriptor. There are situations where this is not appropriate, e.g.
   * a server finished parsing a request but is waiting for some resource
   * to become available before it can send the response back to the client.
   * That is the intent of the _sched_counterop bit. */

  uint8_t flags;

public:
  /* closeonread() and closeonwrite() do not have overloaded mutators
   * because those bits should never be changed after the parsing object
   * is created; they completely change how workers interact with the object. */
  bool closeonread();
  bool closeonwrite();

  bool parse_complete();
  void parse_complete(bool b);
  bool sched_counterop();
  void sched_counterop(bool b);
  
  virtual std::ostream &put_to(std::ostream &o) = 0; // i.e. operator<<
  virtual std::istream &get_from(std::istream &i) = 0; // i.e. operator>>
  virtual Parsing *mkParsing() = 0;

  /* What to do once we've closed the connection. */
  virtual void onclose() {}

  Parsing(bool closeonread, bool closeonwrite,
	  bool parse_complete, bool sched_counterop=true)
    : flags((static_cast<uint8_t>(closeonread) * _closeonread)
	    | (static_cast<uint8_t>(closeonwrite) * _closeonwrite)
	    | (static_cast<uint8_t>(sched_counterop) * _sched_counterop)
	    | (static_cast<uint8_t>(parse_complete) * _parse_complete)) {}
};

/* Since these operators have to be defined outside the class,
 * we just redirect them to dummy functions inside the class. */
std::istream &operator>>(std::istream &i, Parsing &p);
std::ostream &operator<<(std::ostream &o, Parsing &p);

/* A parsing object on the client side should immediately write
 * its request, then close once it has read the response. */
class ClientParsing : public Parsing
{
public:
  ClientParsing() : Parsing(true, false, true) {}
};

/* A parsing object on the server side should read a request,
 * parse it, then close once it has written the response. */
class ServerParsing : public Parsing
{
public:
  ServerParsing() : Parsing(false, true, false) {}
};

#endif // PARSING_H
