#ifndef CALLBACK_HPP
#define CALLBACK_HPP

class Callback
{
public:
  // Are we eventually going to need callbacks with arguments? How to handle?
  virtual void operator()() = 0;
  virtual ~Callback() {}
};

#endif // CALLBACK_HPP
