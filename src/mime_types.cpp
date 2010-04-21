#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <iostream>

#include "logging.h"
#include "mime_types.hpp"

using namespace std;

namespace mime_types
{
  char const *_mime_types::fallback = "application/octet-stream";
  _mime_types::_mime_types()
  {
    fstream f("/etc/mime.types", fstream::in);
    string line;
    while (getline(f, line)) {
      if (line.length() == 0 || line[0] == '#')
	continue;
      stringstream tmp(line);
      string type, extension;
      tmp >> type;
      while (tmp.good()) {
	tmp >> extension;
	types[extension] = type;
      }
    }
    f.close();
  }
  char const *_mime_types::operator()(string const &filename)
  {
    size_t dot;
    if ((dot = filename.rfind('.'))==string::npos)
      return fallback;
    unordered_map<string, string>::iterator it;
    if ((it = types.find(filename.substr(dot+1)))==types.end()) {
      _LOG_DEBUG("not found, returning %s", fallback);
      return fallback;
    }
    return it->second.c_str();
  }
};
