#ifndef HTTP_PARSING_HPP
#define HTTP_PARSING_HPP

#include <iostream>
#include <string>
#include <vector>

// Returns true if we're done parsing, false else.
bool operator>>(std::istream &i, std::vector<std::string> &hdrs);

#endif // HTTP_PARSING_HPP
