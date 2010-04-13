#ifndef HTTP_2616_WORKER_HPP
#define HTTP_2616_WORKER_HPP

#include "Magic_nr.hpp"
#include "Time_nr.hpp"
#include "Worker.hpp"

class HTTP_2616_Worker : public Worker
{
private:
  Magic_nr MIME;
  Time_nr date;
public:
  struct HTTP_2616_Worker_state : public Worker_state
  {
    Magic_nr *MIME;
    Time_nr *date;
    HTTP_2616_Worker_state(Magic_nr *MIME, Time_nr *date)
      : MIME(MIME), date(date) {}
  } mystate;
  HTTP_2616_Worker() : mystate(&MIME, &date) { state = &mystate; }
};

#endif // HTTP_2616_WORKER_HPP
