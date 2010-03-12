#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <functional>
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
  inline void handle_accept();
  inline void handle_sock_err(int fd);

  LockedQueue<Work *> &q;
  
  Work &wmake;
  /* Since the amount of signal handling we currently do is tiny,
   * this could be a vector instead of a hash. w/e */
  std::unordered_map<int, void (*)(int)> sighandlers;
  std::unordered_map<int, void (*)(uint32_t)> special_fd_handlers;

  /* Some ready-made signal handlers. The argument and return types
   * are dictated by the sa_handler field of struct sigaction. (We wouldn't
   * have to do this if we didn't support an alternative to signalfd.) */
  static void flush(int ignore);
  static void halt(int ignore);

  // For use with signal handlers. Ugh...
  static Scheduler *handler_sch;
 
public:
  Scheduler(LockedQueue<Work *> &q, Work &workmaker,
	    int pollsz=100, int maxevents=100);
  void schedule(Work *w, bool oneshot=true);
  void reschedule(Work *w, bool oneshot=true);
  void push_sighandler(int signo, void (*handler)(int));
  bool dowork;
  void set_listenfd(int _listenfd);
  /* Other modules can request that the scheduler pay attention
   * to certain file descriptors without the scheduler having to know
   * how to handle them. For example, a cache module can register
   * an fd with the scheduler that becomes readable whenever a file in the
   * cache changes on disk. */
  void register_special_fd(int fd, void (*cb)(uint32_t),
			   Work::mode m, bool oneshot=true);
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
