#ifndef FINDWORK_HPP
#define FINDWORK_HPP

#include "Work.hpp"

/** \brief Make or find a piece of work.
 * \remarks Somewhat like a Factory for pieces of work, but instead of always
 * returning a new piece of work, we could keep an internal map, returning
 * a pointer to a suitable old piece of work if it already exists. */
class FindWork
{
public:
  FindWork() {}
  /** \brief Find or make a piece of work.
   * \param fd File descriptor of open connection
   * \param m read or write
   * \note If implementations keep an internal map indexed by file descriptor,
   * the second argument might be ignored. */
  virtual Work *operator()(int fd, Work::mode m) = 0;
  /** \brief Register a piece of work that did not come from here initially.
   * \return true if registration was successful, false else (for example,
   * we already had a piece of work with that file descriptor)
   * \remarks Here is the rationale for this function. Servers typically deal with
   * one kind of "piece of work" but occasionally have to deal with another;
   * for example, an HTTP server occasionally has to act as a client to another
   * server. In this case the server would separately create an object
   * representing the client-like work, then register it here with
   * register_alien. */
  virtual bool register_alien(Work *alien) = 0;
  /** \brief Remove piece of work from internal map.
   * \param fd File descriptor of open connection
   * \remarks The intent is for "finished" pieces of work to unregister themselves
   * with the FindWork object, then delete themselves. */
  virtual void unregister(int fd) = 0;
  virtual ~FindWork() {}
private:
  FindWork(FindWork const&);
  FindWork &operator=(FindWork const&);
};

#endif // FINDWORK_HPP
