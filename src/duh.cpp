#include <iostream>

struct wow
{
  int w;
};

class Foo {
public:
  struct : public wow
  {
    int x;
  } s;
};

int main()
{
  Foo f;
  f.s.w = 5;
}
