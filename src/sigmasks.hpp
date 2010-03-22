#ifndef SIGMASKS_HPP
#define SIGMASKS_HPP

#include <vector>

/* Just a tiny wrapper around pthread_sigmask to set the signal mask
 * of the calling thread. The "how" argument should be one of
 * SIG_SETMASK, SIG_BLOCK, SIG_UNBLOCK. */

namespace sigmasks
{
  enum builtin { BLOCK_ALL, BLOCK_NONE };
  void sigmask_caller(builtin b);
  void sigmask_caller(int how, std::vector<int>&sigs);
  void sigmask_caller(int how, int sig);
};

#endif // SIGMASKS_HPP
