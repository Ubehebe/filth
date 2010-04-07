#include <algorithm>
#include <ctype.h>

#include "util.hpp"

using namespace std;

namespace util
{
  string &toupper(string &s)
  {
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
  }
  string &tolower(string &s)
  {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
  }
};
