#include <algorithm>
#include <iostream>
#include <list>

using namespace std;

class Foo
{
  int fake;
public:
  static Foo *mkFoo() { return new Foo(); }
  static list<Foo *> *l;
};

list<Foo *> *Foo::l = NULL;

int main()
{
  list<Foo *> l;
  Foo::l = &l;
 generate_n(l.begin(), 10, Foo::mkFoo);
  
  for (list<Foo *>::iterator it = l.begin(); it != l.end(); ++it)
    delete *it;
  
}
