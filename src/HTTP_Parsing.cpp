#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "CGI_Client_Parsing.h"
#include "HTTP_constants.h"
#include "HTTP_env.h"
#include "HTTP_Parsing.h"
#include "HTTP_Parse_Err.h"

using namespace std;
using namespace HTTP_constants;

// Declare static members.
map<string, pair<time_t, string *> > *HTTP_Parsing::cache;
map<int, pair<stringstream *, Parsing *> > *HTTP_Parsing::state;
LockedQueue<pair<int, char> > *HTTP_Parsing::q;
Scheduler *HTTP_Parsing::sch;

// i.e. operator>>, the main parsing function.
istream &HTTP_Parsing::get_from(istream &i)
{
  string line;
  
  try {
    /* RFC 2616 sec. 5: Request = Request-Line
     * *((general-header | request-header | entity-header) CRLF)
     * CRLF [message-body]
     * I believe we currently ignore the message-body. */
    while (getline(i, line, '\r') && line.length() > 0) {
      if (i.peek() != '\n')
	throw HTTP_Parse_Err(Bad_Request);
      i.ignore();
      (req_line_done)
	? parse_header(line)
	: parse_req_line(line);
    }
    i.clear(); // Since we're at EOF
    parse_complete(true);
    return i;
  }
  catch(HTTP_Parse_Err e) {
    stat = e.stat;
    parse_complete(true);
    return i;
  }
}

// RFC 2616 sec. 5.1: Request-Line = Method SP Request-URI SP HTTP-Version CRLF
void HTTP_Parsing::parse_req_line(string &line)
{
  istringstream tmp(line);
  tmp >> meth;
  string uri;
  tmp >> uri;
  parse_uri(uri);
  string version;
  tmp >> version;
  if (version != HTTP_Version)
    throw HTTP_Parse_Err(HTTP_Version_Not_Supported);
  req_line_done = true;
}

// Parsing for the general-, request-, and entity-headers (RFC 2616, sec. 5)
void HTTP_Parsing::parse_header(string &line)
{
  istringstream tmp(line);
  HTTP_constants::header h;
  try {
    tmp >> h;
  }
  // For now, just ignore headers we don't implement.
  catch (HTTP_Parse_Err e) {
    if (e.stat != Not_Implemented)
      throw e;
  }
}

// i.e. operator<<
ostream &HTTP_Parsing::put_to(ostream &o)
{
  return o << meth << endl << stat << endl;
}

void HTTP_Parsing::parse_uri(string &uri)
{
  // Malformed URI.
  if (uri[0] != '/' || uri.find("..") != string::npos)
    throw HTTP_Parse_Err(Bad_Request);

  string::size_type qpos;
  if (uri == "/") {
    resource_str = HTTP_env::env_vals[HTTP_env::default_resource];
  }
  else if ((qpos = uri.find('?')) != string::npos) {
    resource_str = uri.substr(1, qpos-1);
    query        = uri.substr(qpos+1);
  }
  else {
    resource_str = uri.substr(1);
  }

  /* Check the cache to see if it's in there. If not, do a stat
   * to determine what kind of resource it is (regular file = static,
   * socket file = dynamic). */

  /* TODO: we're actually copying from the cache; pointers would be
   * much more efficient. The problem with that is that dynamic responses
   * currently store their responses _here_, because they get deleted
   * soon thereafter. Perhaps a better way would be to store dynamic
   * responses in the cache and quickly delete them, though that is
   * conceptually questionable. */
  map<string, pair<time_t, string *> >::iterator it;
  if ((it = cache->find(resource_str)) != cache->end())
    response_body = *((*it).second.second);
  else {
    struct stat statbuf;
    if (::stat(resource_str.c_str(), &statbuf)==-1) {
      switch (errno) {
      case ENOENT:
	throw HTTP_Parse_Err(Not_Found);
      case EACCES:
	throw HTTP_Parse_Err(Forbidden);
      default: // Lots more errors we need to address...
	throw HTTP_Parse_Err(Internal_Server_Error);
      }
    }
    
    /* A Unix domain socket, which we interpret as a dynamic resource.
     * The macro S_ISSOCK might test for more than Unix domain sockets,
     * but I don't know how e.g. a traditional TCP socket could get bound
     * in the filesystem. */
    if (S_ISSOCK(statbuf.st_mode)) {
      /* Tell the worker not to schedule the write once it is done parsing:
       * we need to wait until the resource becomes available. */
      sched_counterop(false);
      
      int connfd, flags;
      /* Try to set up a nonblocking connecting socket.
       * Note that if anything goes wrong we report failure back to the client.
       * Should we do something more? */
      if (((connfd = socket(AF_LOCAL, SOCK_STREAM, 0))==-1)
	  || ((flags = fcntl(connfd, F_GETFL))==-1)
	  || (fcntl(connfd, F_SETFL, flags | O_NONBLOCK)==-1))
	throw HTTP_Parse_Err(Internal_Server_Error);

      struct sockaddr_un sa;
      memset((void *)&sa, 0, sizeof(sa));
      sa.sun_family = AF_LOCAL;
      // Note that we may be silently truncating resource_str
      strncpy(sa.sun_path, resource_str.c_str(), sizeof(sa.sun_path)-1);

      // When the connected socket becomes writable, write the query.
      stringstream *s = new stringstream(query);
      CGI_Client_Parsing *p = new CGI_Client_Parsing(q);
      (*state)[connfd] = make_pair(s,p);

      // An immediate connection is the common case.
      if (connect(connfd, (struct sockaddr *) &sa, sizeof(sa))==0)
	q->enq(make_pair(connfd, 'w'));
      // If the connection is still ongoing, have the scheduler watch it.
      else if (errno == EINPROGRESS)
	sch->schedule(connfd, 'w');
      else throw HTTP_Parse_Err(Internal_Server_Error);
    }

    // A regular file, which we interpret as a static resource.
    else if (S_ISREG(statbuf.st_mode)) {
      /* Tell the worker not to schedule a write once it is done parsing;
       * we need to wait for the resource to become available. */
      sched_counterop(false);
      // Start here.
    }

    /* We don't currently support directories, character/block devices,
     * FIFOs, or symlinks. Of these, the only one I am interested in
     * supporting is directories; symlinks sound scary because they could
     * lead outside the server's mount directory. */
    else throw HTTP_Parse_Err(Not_Implemented);
  }
}


Parsing *HTTP_Parsing::mkParsing()
{
  return new HTTP_Parsing();
}
