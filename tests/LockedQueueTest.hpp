#ifndef LOCKED_QUEUE_TEST_HPP
#define LOCKED_QUEUE_TEST_HPP

#include <iostream>
#include <stdint.h>
#include <stdlib.h>

#include "LockedQueue.hpp"
#include "Thread.hpp"

class Producer
{
  LockedQueue<uint16_t> &q;
public:
  Producer(LockedQueue<uint16_t> &q) : q(q) {}
  void produce()
  {
    std::cerr << "producer starting\n";
    uint16_t max = static_cast<uint16_t>(-1);
    for (int i=1; i<=max; ++i)
      q.enq(i);
    std::cerr << "producer done\n";
  }
};

class Consumer
{
  LockedQueue<uint16_t> &q;
  uint16_t last;
public:
  Consumer(LockedQueue<uint16_t> &q) : q(q), last(0) {}
  void consume()
  {
    std::cerr << "consumer starting\n";
    uint16_t tmp, max = static_cast<uint16_t>(-1);
    while (last < max) {
      tmp = q.wait_deq();
      if (tmp != last+1)
	abort();
      last = tmp;
    }
    std::cerr << "consumer done\n";
  }
};

void LockedQueueTest()
{
  LockedQueue<uint16_t> q;
  uint16_t last = 0;
  Producer p(q);
  Consumer c(q);
  Thread<Producer> pthread(&p, &Producer::produce);
  Thread<Consumer> cthread(&c, &Consumer::consume);
}

#endif // LOCKED_QUEUE_TEST_HPP
