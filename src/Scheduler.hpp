#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <functional>
#include <signal.h>
#include <stdint.h>
#include <unordered_map>

#include "Callback.hpp"
#include "ConcurrentQueue.hpp"
#include "logging.h"
#include "FindWork.hpp"

/* N.B. Keep in mind that if the scheduler uses old-fashioned signal
 * handlers, as opposed to signalfd, then any slow system call in the
 * scheduler thread could be interrupted. Presently the only slow
 * system calls used by the scheduler are accept and epoll_wait, but
 * if you add more, remember to check for errno==EINTRs. */
class Scheduler
{
  Scheduler(Scheduler const &);
  Scheduler &operator=(Scheduler const &);

  typedef std::unordered_map<int, void (*)(int)> sighandler_map;
  typedef std::unordered_map<int, Callback *> fdcb_map;
  sighandler_map sighandlers;
  fdcb_map fdcbs;

  struct _acceptcb : public Callback
  {
    int fd;
#ifdef _COLLECT_STATS
    uint32_t accepts;
#endif // _COLLECT_STATS
    Scheduler &sch;
    FindWork &fwork;
    void operator()();
    _acceptcb(Scheduler &sch, FindWork &fwork);
    ~_acceptcb();
  } acceptcb;

  struct _sigcb : public Callback
  {
    int fd;
    Scheduler &sch;
    FindWork &fwork;
    bool &dowork;
    sighandler_map &sighandlers;
    void operator()();
    _sigcb(Scheduler &sch, FindWork &fwork, bool &dowork,
	   sighandler_map &sighandlers)
      : sch(sch), fwork(fwork), dowork(dowork), sighandlers(sighandlers) {}
  } sigcb;

  bool use_signalfd;

  int pollfd, maxevents;
  sigset_t tohandle;

  void handle_sock_err(int fd);

  ConcurrentQueue<Work *> &q;
  FindWork &fwork;

  // For use with signal handlers. Ugh...
  static Scheduler *handler_sch;

public:
  Scheduler(ConcurrentQueue<Work *> &q, FindWork &fwork,
	    int pollsz=100, int maxevents=100);
  ~Scheduler();
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
  void registercb(int fd, Callback *cb, Work::mode m, bool oneshot=true);
  void poll();
  void poisonpill();
  /* Some ready-made signal handlers. The argument and return types
   * are dictated by the sa_handler field of struct sigaction. (We wouldn't
   * have to do this if we didn't support an alternative to signalfd.) */
  static void halt(int ignore);
  static Scheduler *thescheduler; // For non-signalfd-based signal handling
};

#endif // SCHEDULER_HPP
