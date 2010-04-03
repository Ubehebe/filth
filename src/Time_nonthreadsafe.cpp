#include "Time_nonthreadsafe.hpp"

// Man strftime says %z, but I think %Z is right.
char const *Time_nonthreadsafe::RFC822 = "%a, %d %b %Y %H:%M:%S %Z";
