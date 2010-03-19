#ifndef FINDWORK_PREALLOC_HPP
#define FINDWORK_PREALLOC_HPP

#include <stdint.h>

#include "LockedQueue.hpp"
#include "logging.h"

template<size_t SZ> class rawbytes
{
  uint8_t bytes[SZ];
};

/* W is assumed to be a class that has the following declaration:
 * static LockedQueue<void *> store;
 */
template<class W> class FindWork_prealloc : public FindWork
{
public:
  FindWork_prealloc(size_t prealloc_bytes)
  {
    size_t prealloc_chunks = prealloc_bytes / sizeof(W);
  /* This is not great, since we acquire and drop the lock nstore times,
   * however it's just startup code. */
    for (int i=0; i<prealloc_chunks; ++i)
      W::store.enq(reinterpret_cast<void *>(new rawbytes<sizeof(W)>));
  }
  /* Occasionally [just for the standalone cache server currently--can I
   * reproduce this with the web server?], I get a double free/corruption
   * abort here. It doesn't matter for server performance, since this only
   * happens during shutdown, but still...what the hell is going on? */
  ~FindWork_prealloc()
  {
    /* Send everything still in the work map back to the free store.
     * Note that the FindWork destructor will also call clear_Workmap(),
     * but this isn't a problem because by that time the work map is empty. */
    clear_Workmap();

    void *tmp;
    int chunks = 0;
    while (W::store.nowait_deq(tmp)) {
      delete reinterpret_cast<rawbytes<sizeof(W)> *>(tmp);
    }
  }
};

#endif // FINDWORK_PREALLOC_HPP
