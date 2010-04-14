#ifndef HTTP_2616_WORKER_HPP
#define HTTP_2616_WORKER_HPP

#include "Magic_nr.hpp"
#include "Time_nr.hpp"
#include "Worker.hpp"

class HTTP_2616_Worker : public Worker
{
public:
  Magic_nr MIME;
  Time_nr date;
};

#endif // HTTP_2616_WORKER_HPP
