#ifndef HTTP_OOPS_HPP
#define HTTP_OOPS_HPP

#include "HTTP_constants.hpp"
#include "logging.h"
#include "ServerErrs.hpp"

/** \brief Corresponds to the HTTP status values
 * \remarks The intent is to throw an HTTP_oops to short-circuit normal 
 * processing and immediately form a message (usually an error message)
 * back to the client. */
struct HTTP_oops : ServerErr
{
  HTTP_constants::status stat; //!< Not_Found, Internal_Server_Error, etc.
  /** \param stat HTTP status (Not_Found, Internal_Server_Error, etc. */
  HTTP_oops(HTTP_constants::status stat)
    : ServerErr(HTTP_constants::status_strs[stat]), stat(stat)
  {
  }
};


#endif // HTTP_OOPS_HPP

