#include <iostream>
#include <string>
#include <tuple>

int main()
{
  std::tuple<int,int,int> t;
  std::cout << std::get<0>(t) << std::endl;
}

