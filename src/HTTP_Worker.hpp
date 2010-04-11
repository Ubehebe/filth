#ifndef HTTP_WORKER_HPP
#define HTTP_WORKER_HPP

#include "logging.h"
#include "Worker.hpp"

class HTTP_Worker : public Worker
{
public:
  void imbue_state(Work *w) { _LOG_DEBUG("derived!\n"); }
};

template<> class Factory<HTTP_Worker> : public Factory<Worker>
{
public:
  Worker *operator()() { return new HTTP_Worker(); }
};

#endif // HTTP_WORKER_HPP
