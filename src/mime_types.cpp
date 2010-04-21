#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <string.h>

#include "logging.h"
#include "mime_types.hpp"

using namespace std;

namespace mime_types
{
  unordered_map<string, string> _mime_types::types;
  char const *_mime_types::fallback_type;
  void _mime_types::init(char const *mimedb, char const *fallback_type)
  {
    this->fallback_type = fallback_type;
    ifstream f(mimedb, ifstream::in);
    if (f.fail()) {
      _LOG_DEBUG("could not open MIME database %s for reading, exiting",
		 mimedb);
      exit(1);
    }
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
      return fallback_type;
    unordered_map<string, string>::iterator it;
    return ((it = types.find(filename.substr(dot+1)))==types.end())
      ? fallback_type : it->second.c_str();
  }
};
