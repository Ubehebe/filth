#include <iostream>
#include <string>

using namespace std;

void foo(char *x)
{
  *x = 'a';
}

int main()
{
  foo("hi");
}
