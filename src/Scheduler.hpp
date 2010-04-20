#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <functional>
#include <signal.h>
#include <stdint.h>
#include <unordered_map>

#include "Callback.hpp"
#include "ConcurrentQueue.hpp"
#include "FindWork.hpp"

/** \brief The beating heart of a server.
 * \remarks When a connection is first established, it goes into the scheduler
 * until it becomes readable (if it's an incoming, i.e. server-type, connection)
 * or writable (if it's an outgoing, i.e. client-type, connection). It is then
 * put into the job queue. A worker performs the indicated quantum of
 * work, then puts it back into the scheduler if the work isn't finished, where
 * the process begins again. 
 * 
 * The scheduler is also in charge of signal handling, because in recent
 * versions of Linux you can poll for signals just like you poll for ready
 * connections.
 *
 * \todo investigate EPOLLET */
class Scheduler
{
public:
  /** \param q where to enqueue pieces of work ready to be worked on
   * \param listenfd the listening socket, which should already be listening
   * \param fwork how to find/create a piece of work
   * \param pollsz just a hint to the kernel (not a firm limit) about how many
   * connections it should be able to poll at once
   * \param maxevents maximum number of events that will be returned
   * in any one poll. You don't want this too small because of kernel
   * overhead, and you don't want this too big because of latency
   * (the last connection in the list is just sitting there until the scheduler
   * reaches it). */
  Scheduler(ConcurrentQueue<Work *> &q, int listenfd, FindWork *fwork,
	    int pollsz=100, int maxevents=100);
  ~Scheduler();
  /** \brief Start watching a brand-new piece of work.
   * \param w new piece of work
   * \param oneshot if true (default), the piece of work is automatically taken
   * out of the scheduler each time it becomes ready.
   * \warning Your program will probably crash if you try to schedule() a
   * piece of work that is not actually brand-new. Use reschedule() instead.
   * \warning Using oneshot = false undermines the invariant
   * that makes reasoning about the scheduler possible: namely, that a piece
   * of work is only in one "place" at a time. If you use oneshot = false, the
   * following could happen: connection X becomes writable;
   * worker 1 gets connection X and starts writing; connection X becomes
   * writable again; worker 2 gets connection X and starts writing; chaos.
   * Use oneshot=false only when you are putting something into the scheduler
   * that is never really going to be taken out--like the listening socket. */
  void schedule(Work *w, bool oneshot=true);
  /** \brief Re-watch an existing piece of work.
   * \param w existing piece of work
   * \param oneshot if true (default), the piece of work is automatically taken
   * out of the scheduler each time it becomes ready.
   * \warning Your program will probably crash if you try to reschedule() a
   * piece of work that is actually brand-new. Use schedule() instead.
   * \warning Using oneshot = false undermines the invariant
   * that makes reasoning about the scheduler possible: namely, that a piece
   * of work is only in one "place" at a time. If you use oneshot = false, the
   * following could happen: connection X becomes writable;
   * worker 1 gets connection X and starts writing; connection X becomes
   * writable again; worker 2 gets connection X and starts writing; chaos.
   * Use oneshot=false only when you are putting something into the scheduler
   * that is never really going to be taken out--like the listening socket. */
  void reschedule(Work *w, bool oneshot=true);
  /** \brief Register a signal handler.
   * \note the signature of handler is constrained by sigaction. */
  void push_sighandler(int signo, void (*handler)(int));
  bool dowork; //!< Setting to false causes the main loop to fall through
  /** \brief Register a callback tied to a specific file descriptor.
   * For example, a cache module can register an fd with the scheduler
   * that becomes readable whenever a file in the cache changes on disk.
   * I actually had this once, using the inotify API, but took it out once I
   * realized HTTP had its own (worse) cache invalidation mechanisms.
   * \param fd File descriptor. Should not already have been register()ed.
   * \param cb callback
   * \param m read or write
   * \param oneshot if true (default), the piece of work is automatically taken
   * out of the scheduler each time it becomes ready. For this function it
   * might actually make sense to use oneshot = false. */
  void registercb(int fd, Callback *cb, Work::mode m, bool oneshot=true);

  /** \brief Ready-made signal handler: puts a poison pill on the job queue
   * and then retires.
   * \param ignore dummy argument, present because of the function pointer
   * in sigaction */
  static void halt(int ignore=-1);
  static Scheduler *thescheduler; //!< For non-signalfd-based signal handling

private:

  Scheduler(Scheduler const &);
  Scheduler &operator=(Scheduler const &);

  void poll();

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
    FindWork *fwork;
    void operator()();
    _acceptcb(Scheduler &sch, FindWork *fwork, int listenfd);
    ~_acceptcb();
  } acceptcb;

  struct _sigcb : public Callback
  {
    int fd;
    Scheduler &sch;
    FindWork *fwork;
    bool &dowork;
    sighandler_map &sighandlers;
    void operator()();
    _sigcb(Scheduler &sch, FindWork *fwork, bool &dowork,
	   sighandler_map &sighandlers)
      : sch(sch), fwork(fwork), dowork(dowork), sighandlers(sighandlers) {}
  } sigcb;

  bool use_signalfd;

  int pollfd, maxevents;
  sigset_t tohandle;

  ConcurrentQueue<Work *> &q;

  /* The scheduler needs to know about this to make new work. */
  FindWork *fwork;
};

#endif // SCHEDULER_HPP
