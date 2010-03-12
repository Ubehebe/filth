#include <algorithm>
#include <iostream>
#include <list>

using namespace std;

class Foo
{
public:
  Foo() { cout << "Foo\n"; }
};

class Bar
{
public:
  Bar() { cout << "Bar\n"; }
};

class Baz : public Bar
{
  Foo f;
public:
  Baz() { cout << "Baz\n"; }
};

int main()
{
  Baz();
}
