#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <strings.h>
#include <syslog.h>

#include "Compressor.hpp"

using namespace std;

int main(int argc, char **argv)
{
  if (argc != 2) {
    cout << "usage: " << argv[0] << " <infile>\n";
    return 1;
  }

  fstream f;
  string tmp;
  f.open(argv[1], ios::in);
  f.seekg(0, ios::end);
  size_t sz1 = f.tellg();
  size_t sz2 = sz1 + 1<<10;
  size_t sz3 = sz1;
  f.seekg(0, ios::beg);
  char *buf1 = new char[sz1];
  char *buf2 = new char[sz2];
  char *buf3 = new char[sz3];
  f.read(buf1, sz1);
  f.close();

  if (!Compressor::compress(buf2, sz2, buf1, sz1))
    abort();
  if (!Compressor::uncompress(buf3, sz3, buf2, sz2))
    abort();
  if (sz3 != sz1 || strncmp(buf1, buf3, sz1) != 0)
    abort();

  delete[] buf3;
  delete[] buf2;
  delete[] buf1;
}
