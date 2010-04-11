#include "Time_nr.hpp"

// Man strftime says %z, but I think %Z is right.
char const *Time_nr::RFC822 = "%a, %d %b %Y %H:%M:%S %Z";
