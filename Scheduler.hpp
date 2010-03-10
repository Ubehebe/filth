#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <functional>
#include <map>
#include <signal.h>
#include <unordered_map>
#include "LockedQueue.hpp"
#include "Work.hpp"

/* N.B. Keep in mind that if the scheduler uses old-fashioned signal
 * handlers, as opposed to signalfd, then any slow system call in the
 * scheduler thread could be interrupted. Presently the only slow
 * system calls used by the scheduler are accept and epoll_wait, but
 * if you add more, remember to check for errno==EINTRs. */
class Scheduler
{
  // No copying, no assigning.
  Scheduler(Scheduler const &);
  Scheduler &operator=(Scheduler const &);

  bool use_signalfd;

  int listenfd, pollfd, sigfd, maxevents;
  sigset_t tohandle;

  inline void handle_sigs();
  inline void handle_listen();
  inline void handle_sock_err(int fd);

  LockedQueue<Work *> &q;
  std::unordered_map<int, Work *> &state;
  
  mkWork &makework;
  /* This doesn't really need to be an ordered map, but since
   * the amount of signal handling we currently do is tiny, I'm not
   * motivated to make it better. */
  std::map<int, void (*)(int)> sighandlers;

  /* Some ready-made signal handlers. The argument and return types
   * are dictated by the sa_handler field of struct sigaction. (We wouldn't
   * have to do this if we didn't support an alternative to signalfd.) */
  static void flush(int ignore);
  static void halt(int ignore);

  // For use with signal handlers. Ugh...
  static Scheduler *handler_sch;
 
public:
  Scheduler(LockedQueue<Work *> &q,
	    std::unordered_map<int, Work *> &state,
	    mkWork &makework, int pollsz=100, int maxevents=100);
  void schedule(Work *w, bool oneshot=true);
  void reschedule(Work *w, bool oneshot=true);
  void push_sighandler(int signo, void (*handler)(int));
  bool dowork;
  void set_listenfd(int _listenfd);
  void poll();


};

/* To support signal handling without the use of signalfd, we need
 * a static scheduler object. I tried creating one inside the scheduler class
 * (the obvious place for it to go), but was stymied by linker errors. */
namespace handler_sch
{
  extern Scheduler *s;
};

#endif // SCHEDULER_HPP
