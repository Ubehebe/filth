#include <iostream>

class Foo
{
  int x,y;
public:
  ~Foo();
  ~Foo(int x);
};

int main()
{
  std::cout << sizeof(Foo) << std::endl;
}
