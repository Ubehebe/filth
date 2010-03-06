#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <map>
#include <signal.h>
#include "LockedQueue.h"
#include "Work.h"

class Scheduler
{
  // No copying, no assigning.
  Scheduler(Scheduler const &);
  Scheduler &operator=(Scheduler const &);

  int listenfd, pollfd, sigfd, maxevents;
  sigset_t tohandle;

  inline void handle_sigs();
  inline void handle_listen();
  inline void handle_sock_err(int fd);

  LockedQueue<Work *> &q;
  mkWork &makework;
  std::map<int, void (*)(Scheduler *)> sighandlers;

  // Some ready-made signal handlers.
  static void flush(Scheduler *s);
  static void halt(Scheduler *s);
 
public:
  Scheduler(LockedQueue<Work *> &q, mkWork &makework,
	    int pollsz=100, int maxevents=100);
  void schedule(Work *w, bool oneshot=true);
  void reschedule(Work *w, bool oneshot=true);
  void push_sighandler(int signo, void (*handler)(Scheduler *));
  bool dowork;
  void set_listenfd(int _listenfd);
  void poll();
};

#endif // SCHEDULER_H
