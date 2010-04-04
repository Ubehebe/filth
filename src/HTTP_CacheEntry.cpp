#include "HTTP_CacheEntry.hpp"
#include "logging.h"

inline time_t HTTP_CacheEntry::now()
{
  return ::time(NULL);
}

inline time_t HTTP_CacheEntry::corrected_received_age()
{
  time_t tmp = now() - date_value;
  return (tmp > age_value) ? tmp : age_value;
}

inline time_t HTTP_CacheEntry::response_delay()
{
  return response_time - request_time;
}


inline time_t HTTP_CacheEntry::corrected_initial_age()
{
  return corrected_received_age() + response_delay();
}

inline time_t HTTP_CacheEntry::resident_time()
{
  return now() - response_time;
}

inline time_t HTTP_CacheEntry::current_age()
{ 
  return corrected_initial_age() + resident_time();
}

inline time_t HTTP_CacheEntry::freshness_lifetime()
{
  return (use_max_age) ? max_age_value : expires_value - date_value;
}

inline bool HTTP_CacheEntry::response_is_fresh()
{
  return freshness_lifetime() > current_age();
}
