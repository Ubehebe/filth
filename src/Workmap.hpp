#ifndef WORKMAP_HPP
#define WORKMAP_HPP

#include <unordered_map>

#include "Work.hpp"

/* This data structure is unsynchronized. I believe this is safe since
 * two workers never have the same file descriptor, but I need to think
 * about it more. */
typedef std::unordered_map<int, Work *> Workmap;

#endif // WORKMAP_HPP
