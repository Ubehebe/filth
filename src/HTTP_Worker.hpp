#ifndef HTTP_WORKER_HPP
#define HTTP_WORKER_HPP

#include "logging.h"
#include "Magic_nr.hpp"
#include "Time_nr.hpp"
#include "Worker.hpp"

class HTTP_Worker : public Worker
{
  Magic_nr MIME;
  Time_nr date;
public:
  void imbue_state(Work *w)
  {
    // We know we're working with HTTP_Server_Work_big objects...right?
    reinterpret_cast<HTTP_Server_Work_big *>(w)->MIME = &MIME;
    reinterpret_cast<HTTP_Server_Work_big *>(w)->date = &date;
  }
};

template<> class Factory<HTTP_Worker> : public Factory<Worker>
{
public:
  Worker *operator()() { return new HTTP_Worker(); }
};

#endif // HTTP_WORKER_HPP
