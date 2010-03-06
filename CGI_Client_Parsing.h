#ifndef CGI_CLIENT_PARSING_H
#define CGI_CLIENT_PARSING_H

#include <iostream>
#include <string>
#include "HTTP_Parsing.h"
#include "Parsing.h"
#include "LockedQueue.h"

class CGI_Client_Parsing : public ClientParsing
{
  /* The client that requested this dynamic resource,
   * so we can write back to it after we have received it. */
  int clientfd;
  // Where to put the response.
  HTTP_Parsing *client_state;
  std::string response;

  static LockedQueue<std::pair<int, char> > *q;
public:
  CGI_Client_Parsing(LockedQueue<std::pair<int, char> > *_q) { q = _q; }
  std::ostream &put_to(std::ostream &o);
  std::istream &get_from(std::istream &i);
  Parsing *mkParsing() { return new CGI_Client_Parsing(q); }
  void onclose();

};

#endif // CGI_CLIENT_PARSING_H
