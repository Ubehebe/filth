#ifndef SERVER_ERR_H
#define SERVER_ERR_H

struct ServerErr
{
  char const *msg;
  ServerErr(char const *msg) : msg(msg) {}
};

// Out of memory, too many files open, etc.
struct ResourceErr : ServerErr
{
  int err;
  ResourceErr(char const *msg, int err) : ServerErr(msg), err(err) {}
};

// Bind errors, listen errors, socket read/write errors, etc.
struct SocketErr : ServerErr
{
  int err;
  SocketErr(char const *msg, int err) : ServerErr(msg), err(err) {}
};

#endif // SERVER_ERR_H
