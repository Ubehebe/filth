#ifndef HTTP_OOPS_HPP
#define HTTP_OOPS_HPP

#include "HTTP_constants.hpp"
#include "logging.h"
#include "ServerErrs.hpp"

struct HTTP_oops : ServerErr
{
  HTTP_constants::status stat;
  HTTP_oops(HTTP_constants::status stat)
    : ServerErr(HTTP_constants::status_strs[stat]), stat(stat)
  {
    _LOG_DEBUG("%s", HTTP_constants::status_strs[stat]);
  }
};


#endif // HTTP_OOPS_HPP

