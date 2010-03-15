#include "Parsing.h"

std::istream &operator>>(std::istream &i, Parsing &p) { return p.get_from(i); }
std::ostream &operator<<(std::ostream &o, Parsing &p) { return p.put_to(o); }

bool Parsing::closeonread()
{
  return static_cast<bool>(flags & _closeonread);
}

bool Parsing::closeonwrite()
{ 
  return static_cast<bool>(flags & _closeonwrite);
}

bool Parsing::parse_complete() 
{ 
  return static_cast<bool>(flags & _parse_complete);
}

void Parsing::parse_complete(bool b)
{
  flags = (b) ? flags | _parse_complete : flags & (~_parse_complete);
}

bool Parsing::sched_counterop()
{ 
  return static_cast<bool>(flags & _sched_counterop);
}

void Parsing::sched_counterop(bool b)
{
  flags = (b) ? flags | _sched_counterop : flags & (~_sched_counterop);
}
