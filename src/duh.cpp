#include <stdio.h>

class Foo
{
public:
  virtual void operator()(int blah) =0;
};

class Bar : public Foo
{
public:
  void operator()(int blah=44)
  {
    printf("%d\n", blah);
  }
};

int main()
{
  Bar b;
  b();
}
