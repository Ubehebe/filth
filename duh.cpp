#include <stdio.h>

#include "Thread.hpp"

class Foo
{
public:
  void blah() {}
};

int main()
{
  for (int i=0; i< 100; ++i)
    Thread<Foo> th(&Foo::blah);
}
