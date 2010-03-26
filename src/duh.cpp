#include <iostream>
#include <string>

// Just a git test!

using namespace std;

template<class T> class Factory
{
public:
  T *operator()();
};

template<> class Factory<string>
{
  string touse;
public:
  Factory(char const *s) : touse(s) {}
  string *operator()() { return new string(touse); }
};

int main()
{
  Factory<string> f("hi!");
  cout << *(f()) << endl;
}

