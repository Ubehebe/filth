#ifndef SERVER_ERR_HPP
#define SERVER_ERR_HPP

/** \brief General-purpose server error. */
struct ServerErr
{
  char const *msg;
  ServerErr(char const *msg) : msg(msg) {}
};

/** \brief Out of memory, too many files open, etc. */
struct ResourceErr : ServerErr
{
  int err;
  ResourceErr(char const *msg, int err) : ServerErr(msg), err(err) {}
};

/** \brief Bind errors, listen errors, socket read/write errors, etc. */
struct SocketErr : ServerErr
{
  int err;
  SocketErr(char const *msg, int err) : ServerErr(msg), err(err) {}
};

#endif // SERVER_ERR_HPP
