#ifndef HTTP_PARSE_ERR_HPP
#define HTTP_PARSE_ERR_HPP

#include "HTTP_constants.h"
#include "ServerErrs.h"

struct HTTP_Parse_Err : ServerErr
{
  HTTP_constants::status stat;
  HTTP_Parse_Err(HTTP_constants::status stat)
    : ServerErr(HTTP_constants::status_strs[stat]), stat(stat) {}
};


#endif // HTTP_PARSE_ERR_HPP

