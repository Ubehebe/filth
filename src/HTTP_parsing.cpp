#include <sstream>

#include "HTTP_constants.hpp"
#include "HTTP_Parse_Err.hpp"
#include "HTTP_parsing.hpp"
#include "logging.h"

using namespace std;
using namespace HTTP_constants;

bool operator>>(istream &input, structured_hdrs_type &hdrs)
{
  string line;

  // Get a line until there are no more lines, or we hit the empty line.
  while (getline(input, line, '\r') && line.length() > 0) {
    /* If the line isn't properly terminated, save it and report that we
     * need more text from the client in order to parse. */
    if (input.peek() != '\n') {
      input.clear();
      input.seekg(-line.length(), ios::cur);
      return false;
    }
    input.ignore(); // Ignore the \n (the \r is already consumed)
    input.clear();

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
      } catch (HTTP_Parse_Err e) {} // Just ignore unrecognized headers
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

std::ostream &operator<<(std::ostream &o, structured_hdrs_type &hdrs)
{
  // We count down instead of up because the request line is stored at the end.
  for (int h=reqln; h >=0; --h) {
    if (!hdrs[h].empty())
      o << hdrs[h];
  }
  return o;
}
