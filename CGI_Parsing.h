#ifndef CGI_PARSING_H
#define CGI_PARSING_H

#include <iostream>
#include "Parsing.h"

class CGI_Parsing : public Parsing
{
public:
  std::ostream &put_to(std::ostream &o) { return o; }
  std::istream &get_from(std::istream &i) { return i; }
  Parsing *mkParsing() { return new CGI_Parsing(); }
};

#endif // CGI_PARSING_H
