#ifndef SIGMASKS_HPP
#define SIGMASKS_HPP

#include <pthread.h>
#include <vector>

/** \brief Just a tiny wrapper around pthread_sigmask to set the signal mask
 * of the calling thread. */
namespace sigmasks
{
  /** \brief Aliases for commonly used signal masks. */
  enum builtin { BLOCK_ALL, BLOCK_NONE };
  /** \param b BLOCK_ALL or BLOCK_NONE */
  void sigmask_caller(builtin b);
  /** \param how one of SIG_SETMASK, SIG_BLOCK, SIG_UNBLOCK (see
   * pthread_sigmask documentation)
   * \param sigs vector of signals, just an alternative to the sigset interface
   */
  void sigmask_caller(int how, std::vector<int>&sigs);
  /** \param how one of SIG_SETMASK, SIG_BLOCK, SIG_UNBLOCK (see
   * pthread_sigmask documentation)
   * \param sig single signal to add/remove/change */
  void sigmask_caller(int how, int sig);
  /** \param how one of SIG_SETMASK, SIG_BLOCK, SIG_UNBLOCK (see
   * pthread_sigmask documentation)
   * \param sigset see pthread_sigmask documentation */
  void sigmask_caller(int how, sigset_t *sigset);
};

#endif // SIGMASKS_HPP
