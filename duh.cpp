#include "HTTP_Server.h"

struct Foo
{
  static int _id;
  int id;
  Foo(int x) : id(x) {}
};

int Foo::_id = 0;

int main()
{
  Foo fs[10](5);
  for (int i=0; i < 10; ++i)
    std::cout << fs[i].id << std::endl;
}
