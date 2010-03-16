#include <iostream>

struct Foo
{
  struct foo
  {
    int f;
    virtual ~foo() {}
  };
  virtual foo *mkfoo() { return new foo(); }
  foo *getfoo() { return mkfoo(); }
};

struct Bar : Foo
{
  struct bar : foo
  {
    int b;
    bar() : b(43) {}
  };
  foo *mkfoo() { return new bar(); }
};

int main()
{
  Bar b;
  std::cout << dynamic_cast<Bar::bar *>(b.getfoo())->b << std::endl;
}
