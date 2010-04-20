#include <sstream>

#include "HTTP_constants.hpp"
#include "HTTP_oops.hpp"
#include "HTTP_parsing.hpp"
#include "logging.h"

using namespace std;
using namespace HTTP_constants;

namespace HTTP_parsing
{

  /** \brief When to stop parsing a line and report an error.
   * Default value of -1 means this is disabled. */
  static size_t max_line_len = -1;

  void setmaxes(size_t const &max_line_len)
  {
    HTTP_parsing::max_line_len = max_line_len;
  }

  bool operator>>(istream &input, structured_hdrs_type &hdrs)
  {
    string line;

    // Get a line until there are no more lines, or we hit the empty line.
    while (getline(input, line, '\r') && line.length() > 0) {
      _LOG_DEBUG("%s", line.c_str());
      if (max_line_len != -1 && line.length() > max_line_len)
	throw HTTP_oops(Bad_Request);
      /* If the line isn't properly terminated, save it and report that we
       * need more text from the client in order to parse. */
      if (input.peek() != '\n') {
	input.clear();
	input.seekg(-line.length(), ios::cur);
	return false;
      }
      input.ignore(); // Ignore the \n (the \r is already consumed)
      input.clear();

      // Note that this covers both request lines and response status lines
      if (line.find(HTTP_Version) != line.npos) {
	hdrs[reqln] = line;
      }
      else {
	stringstream tmp(line);
	header h;
	try {
	  tmp >> h;
	  int hi = static_cast<int>(h);
	  hdrs[hi] = line;
	} catch (HTTP_oops e) { } // silently ignore headers we don't understand
      }
    }
    input.clear();
    // We got to the empty line.
    if (line.length() == 0 && input.peek() == '\n') {
      input.ignore();
      return true;
    } else {
      _LOG_DEBUG("line not properly terminated: %s", line.c_str());
      input.clear();
      input.seekg(-line.length(), ios::cur);
      return false;
    }
  }

  ostream &operator<<(ostream &o, structured_hdrs_type const &hdrs)
  {
    // We count down instead of up because the request line is stored at the end.
    for (int h=reqln; h >=0; --h) {
      if (!hdrs[h].empty()) {
	o << hdrs[h] << CRLF;
      }
    }
    return o << CRLF;
  }

};
