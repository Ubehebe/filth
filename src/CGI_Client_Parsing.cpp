#include "CGI_Client_Parsing.h"

using namespace std;

LockedQueue<std::pair<int, char> > *CGI_Client_Parsing::q;

// Not a real parsing...
ostream &CGI_Client_Parsing::put_to(ostream &o)
{
  return o;
}

istream &CGI_Client_Parsing::get_from(istream &i)
{
  string line;
  
  /* Very simple for now: just copy the CGI server's response to a buffer. */
  while (getline(i, line, '\r') && line.length() > 0)
    response += line;
  i.clear(); // Since we're at EOF
  parse_complete(line.length() == 0);
  return i;
}

void CGI_Client_Parsing::onclose()
{
  client_state->response_body = response;
  q->enq(make_pair(clientfd, 'w'));
}
