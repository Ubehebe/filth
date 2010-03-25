#include <iostream>
#include <string>

using namespace std;

template<class T> class Factory
{
public:
  T *operator()();
};

template<> class Factory<string>
{
public:
  string *operator()() { return new string("hello"); }
};

int main()
{
  Factory<string> f;
  cout << *(f()) << endl;
}

