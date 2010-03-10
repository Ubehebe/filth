#include <iostream>

void foo() {}
void foo(int x, int y) {}

int main()
{
  int x = 5;
  int *y = &x;
  int *&z = y;
  std::cout << *y << std::endl;
}
