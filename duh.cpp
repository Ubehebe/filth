#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>

using namespace std;

int main()
{
  stringstream str;
  str << "hello, world";
  string line;
  getline(str, line);
  cout << line << endl;
  str.str("");
  cout << str.str() << endl;
}
