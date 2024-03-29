#ifndef WORK_HPP
#define WORK_HPP

#include <iostream>
#include <stdint.h>

// Forward declarations for pointers
class Worker;

/** \brief A "unit of work" in a client or server.
 * \remarks A "unit of work" is some reading or writing that needs to be done
 * at an open file descriptor. A worker gets a unit of work and executes it
 * via the operator(). By the time the worker is done executing the piece
 * of work, it has either been rescheduled for another read/write,
 * is "done", so that the worker may delete it, or has gone dormant,
 * waiting for the asynchronous arrival of a resource it needs. */
class Work
{
public:
  /** \brief Should we read or write? */
  enum mode { read, write } m;
  int fd; //!< File descriptor of socket, file, etc.
  bool deleteme; //!< Whether the worker should delete this piece of work.
  /** \param fd open connection
   * \param m should the first action be a read or a write?
   * \param deleteme should the worker delete me (and close my connection)
   * when I'm done?
   * \param islisten if I am the listening socket, I should never be closed */
  Work(int fd, mode m, bool deleteme=false, bool islisten=false);
  /** \brief The only function seen by the worker.
   * \param w the current worker, used to pass worker-specific state to the
   * piece of work. */
  virtual void operator()(Worker *w) = 0;
  virtual ~Work();
  /** \brief Let all work objects know who the listening socket is, so they don't
   * close it.
   * \param listenfd the listening socket */
  static void setlistenfd(int listenfd);
protected:
  /** \brief Read from the socket until we would block.
   * \param inbuf where to store the input
   * \param rdbuf C character array used for low-level reading
   * \param rdbufsz size of rdbuf
   * \return reason for stopping the read (errno codes). Will never return
   * 0 or EINTR.
   * \note This function is suitable when you don't know in advance when
   * reading should stop (e.g., reading HTTP headers, which only end after a
   * double CRLF). The intent is to check inbuf after every return to determine
   * whether reading is complete. */
  int rduntil(std::ostream &inbuf, uint8_t *rdbuf, size_t rdbufsz);
  /** \brief Try to read specified number of bytes from socket into string.
   * \param s where to store the input
   * \param rdbuf C character array used for low-level reading
   * \param rdbufsz size of rdbuf
   * \param tord number of bytes to read (on return, contains number of bytes
   * actually read)
   * \return 0 if we successfully read all tord bytes, a read errno value else
   * \note This function is suitable when you know in advance when
   * reading should stop (e.g. reading HTTP bodies, whose size should be
   * given in advance by the Content-Length header). */
  int rduntil(std::string &s, uint8_t *rdbuf, size_t rdbufsz, size_t &tord);
  /** \brief Try to write specified number of bytes to socket.
   * \param outbuf pointer to data to write
   * \param towrite number of bytes to write (on return, contains number of
   * bytes actually written)
   * \return 0 if we successfully wrote all towrite bytes, a write errno value
   * else */
  int wruntil(uint8_t const *&outbuf, size_t &towrite);



private:
  /* Never close the listening socket, even though its associated data might
   * be erased. The reason is because once closed, if there are pending
   * connections the socket will enter the dreaded TIME_WAIT state,
   * and attempts to bind to the same address will fail for several minutes.
   * For applications that can use ephemeral ports this can be overcome
   * with the SO_REUSEADDR socket option, but this does not work for
   * applications that always have to bind to a well-known port: the re-bind
   * will fail even with SO_REUSEADDR turned on. */
  static int listenfd;
  Work(Work const&);
  Work &operator=(Work const&);
};

#endif // WORK_HPP
