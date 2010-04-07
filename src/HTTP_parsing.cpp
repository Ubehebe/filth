#include <sstream>

#include "HTTP_constants.hpp"
#include "HTTP_parsing.hpp"
#include "logging.h"

using namespace std;
using namespace HTTP_constants;

bool operator>>(istream &input, vector<string> &hdrs)
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

    if (line.find(HTTP_Version) != line.npos) {
      hdrs[reqln] = line;
    }

    else {
      stringstream tmp(line);
      header h = static_cast<header>(num_header);
      tmp >> h;
      int hi = static_cast<int>(h);
      if (hi != num_header)
	hdrs[hi] = line;
    }
  }
  input.clear();
  // We got to the empty line.
  if (line.length() == 0 && input.peek() == '\n') {
    return true;
  } else {
    _LOG_DEBUG("line not properly terminated: %s", line.c_str());
    input.clear();
    input.seekg(-line.length(), ios::cur);
    return false;
  }
}
