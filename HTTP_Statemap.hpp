#ifndef HTTP_STATEMAP_HPP
#define HTTP_STATEMAP_HPP

#include <unordered_map>

#include "Work.hpp"

/* This data structure is unsynchronized. I believe this is safe since
 * two workers never have the same file descriptor, but I need to think
 * about it more. */
typedef std::unordered_map<int, Work *> HTTP_Statemap;

#endif // HTTP_STATEMAP_HPP
