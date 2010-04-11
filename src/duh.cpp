#include <iostream>
#include <string>

#include "Factory.hpp"
#include "ThreadPool.hpp"

/* THIS WORKS!!! */

class Foo
{
public:
  virtual void dosth() { std::cout << "Foo\n"; }
};

class Bar : public Foo
{
public:
  void dosth() { std::cout << "Bar\n"; }
};

template<> class Factory<Foo>
{
public:
  virtual Foo *operator()() { return new Foo(); }
};

template<> class Factory<Bar> : public Factory<Foo>
{
public:
  Foo *operator()() { return new Bar(); }
};

int main()
{
  Factory<Bar> bf;
  ThreadPool<Foo> tp(bf, &Foo::dosth, 5);
  tp.start();
}

