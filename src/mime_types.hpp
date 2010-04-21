#ifndef MIME_TYPES_HPP
#define MIME_TYPES_HPP

#include <unordered_map>

namespace mime_types
{
  class _mime_types
  {
    static char const *fallback;
    std::unordered_map<std::string, std::string> types;
  public:
    _mime_types();
    char const *operator()(std::string const &filename);
  };
  static _mime_types m;
}

#endif // MIME_TYPES_HPP
