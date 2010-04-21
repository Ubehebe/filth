#ifndef HTTP_2616_WORKER_HPP
#define HTTP_2616_WORKER_HPP

#include "Time_nr.hpp"
#include "Worker.hpp"

/** \brief A thin layer on top the the generic worker, adding some lookup
 * resources.
 * \remarks In a full HTTP server, there are a few resources that I think it
 * makes sense to have Worker- (i.e. thread-) specific, rather than
 * Work-specific (too much allocation) or shared among all threads/workers
 * (too much contention). For example, time and MIME lookups. */
class HTTP_2616_Worker : public Worker
{
public:
  Time_nr date; //!< Format time.
};

#endif // HTTP_2616_WORKER_HPP
