#ifndef CACHE_HPP
#define CACHE_HPP

#include <list>
#include <unordered_map>
#include <unordered_set>

#include "Locks.hpp"
#include "logging.h"

/* TODO: learn more about pointer specializations for templates. */
/* TODO: repurpose instrumentation from the old cache classes. */

template<class Handle, class Stuff, bool Stuff_is_ptr=true> class Cache
{
  // Wrapper to hold reference counts and sizes.
  struct _Stuff
  {
    Stuff s;
    int refcnt;
    bool invalid;
    size_t sz;
    _Stuff(Stuff s, size_t sz) : s(s), refcnt(1), invalid(false), sz(sz) {}
    ~_Stuff() { if (Stuff_is_ptr) delete s; }
  };

  // TODO: replace with amazing lock-free variant
  typedef std::unordered_map<Handle, _Stuff *> cache_type;
  RWLock cachelock;
  cache_type cache;

  // TODO: replace with amazing lock-free variant
  typedef std::unordered_set<Handle> evict_type;
  Mutex evictlock;
  evict_type toevict;

  size_t cur, max;

  bool evict();
  bool evict(Handle &h);

public:
  bool put(Handle &h, Stuff &s, size_t sz);
  bool get(Handle &h, Stuff &s);
  void unget(Handle &h);
  void flush();
  // Clients can advise an eviction but not force it.
  void advise_evict(Handle &h);
  Cache(size_t sz);
  ~Cache();
};

template<class Handle, class Stuff, bool Stuff_is_ptr>
Cache<Handle, Stuff, Stuff_is_ptr>::Cache(size_t max)
  : cur(0), max(max)
{
}

template<class Handle, class Stuff, bool Stuff_is_ptr>
Cache<Handle, Stuff, Stuff_is_ptr>::~Cache()
{
  flush();
}

template<class Handle, class Stuff, bool Stuff_is_ptr>
bool Cache<Handle, Stuff, Stuff_is_ptr>::get(Handle &h, Stuff &s)
{
  bool ans;
  typename cache_type::iterator it;
  cachelock.rdlock();
  if (ans = (it = cache.find(h)) != cache.end()) {
    __sync_fetch_and_add(&it->second->refcnt, 1);
    s = it->second->s;
  }
  cachelock.unlock();
  return ans;
}

template<class Handle, class Stuff, bool Stuff_is_ptr>
void Cache<Handle, Stuff, Stuff_is_ptr>::unget(Handle &h)
{
  typename cache_type::iterator it;
  bool doenq, doevict;
  cachelock.rdlock();
  doenq = ((it = cache.find(h)) != cache.end()
	   && __sync_sub_and_fetch(&it->second->refcnt, 1)==0);
  doevict = doenq && it->second->invalid;
  cachelock.unlock();
  
  /* If doenq is true but doevict is false, we enqueue for later eviction.
   * If doenq is true and doevict is true, we try to evict immediately,
   * and if that fails, we enqueue for later eviction. */
  if (doenq && !(doevict && evict(h))) {
    evictlock.lock();
    toevict.insert(h);
    evictlock.unlock();
  }
}

template<class Handle, class Stuff, bool Stuff_is_ptr>
bool Cache<Handle, Stuff, Stuff_is_ptr>::put(Handle &h, Stuff &s, size_t sz)
{
  if (sz > max)
    return false;

  bool already;
  cachelock.rdlock();
  already = (cache.find(h) != cache.end());
  cachelock.unlock();
  if (already)
    return false;

 put_tryagain:
  if (__sync_add_and_fetch(&cur, sz) > max) {
    __sync_sub_and_fetch(&cur, sz);
    if (evict())
      goto put_tryagain;
    else
      return false;
  }

  _Stuff *_s = new _Stuff(s, sz);

  cachelock.wrlock();
  already = (cache.find(h) != cache.end());
  if (!already)
    cache[h] = _s;
  cachelock.unlock();
  return !already;
}

template<class Handle, class Stuff, bool Stuff_is_ptr>
void Cache<Handle, Stuff, Stuff_is_ptr>::flush()
{
  std::list<_Stuff *> l;
  cachelock.wrlock();
  for (typename cache_type::iterator it = cache.begin(); it != cache.end(); ++it)
    l.push_back(it->second);
  for (typename std::list<_Stuff *>::iterator it = l.begin(); it != l.end(); ++it)
    delete *it;
  cur = 0;
  cache.clear();
  cachelock.unlock();
}

template<class Handle, class Stuff, bool Stuff_is_ptr>
bool Cache<Handle, Stuff, Stuff_is_ptr>::evict()
{
  Handle h;
  do {
    evictlock.lock();
    if (toevict.empty()) {
      evictlock.unlock();
      return false;
    }
    typename evict_type::iterator it = toevict.begin();
    h = *it;
    toevict.erase(it);
    evictlock.unlock();
  } while (!evict(h));

  return true;
}

template<class Handle, class Stuff, bool Stuff_is_ptr>
bool Cache<Handle, Stuff, Stuff_is_ptr>::evict(Handle &h)
{
  typename cache_type::iterator it;
  bool evicted = false;

  cachelock.wrlock();

  if ((it = cache.find(h)) != cache.end() && it->second->refcnt == 0) {
    __sync_sub_and_fetch(&cur, it->second->sz);
    delete it->second;
    cache.erase(it);
    evicted = true;
  }

  cachelock.unlock();

  return evicted;
}

template<class Handle, class Stuff, bool Stuff_is_ptr>
void Cache<Handle, Stuff, Stuff_is_ptr>::advise_evict(Handle &h)
{
  typename cache_type::iterator it;
  cachelock.rdlock();
  if ((it = cache.find(h)) != cache.end())
    __sync_fetch_and_or(&it->second->invalid, true);
  cachelock.unlock();
}


#endif // CACHE_HPP
