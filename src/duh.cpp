#include <iostream>

class Foo
{
protected:
  int x;
public:
  Foo() : x(13) {}
};

class Bar : protected Foo
{
  void oops() { std::cout << Foo::x << std::endl; }
public:
  int b;
  Bar() : b(45) {}

};

int main()
{
  Bar b;
}
