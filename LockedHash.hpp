#ifndef LOCKED_HASH_HPP
#define LOCKED_HASH_HPP

#include <unordered_map>

#include "Locks.hpp"

/* Simple locked hash table using a reader/writer lock.
 * The interface is a bit different from the STL interfaces; in particular,
 * since I don't know enough about the invalidation of iterators,
 * we don't expose iterators at all. */
template<class K, class V> class LockedHash
{
  std::unordered_map<K, V> m;
  RWLock lock;
public:
  bool insert(std::pair<K,V> p, V &v);
  bool find(K k, V &v);
  bool pop(K k, V &v);
  bool popif(K k, V &v, bool (*cond)(V v));
};

using namespace std;

template<class K, class V> bool
LockedHash<K,V>::insert(pair<K,V> p, V &v)
{
  bool ans = false;
  lock.wrlock();
  /* TODO: use iterators to do one lookup, not two. But I get weird syntax
   * errors with iterators... */
  if (m.find(p.first) == m.end()) {
    m.insert(p);
    ans = true;
  }
  else {
    v = m[p.first];
  }
  lock.unlock();
  return ans;
}

template<class K, class V> bool LockedHash<K,V>::find(K k, V &v)
{
  bool ans = false;
  lock.rdlock();
  /* TODO: use iterators to do one lookup, not two. But I get weird syntax
   * errors with iterators... */
  if (m.find(k) != m.end()) {
    v = m[k];
    ans = true;
  }
  lock.unlock();
  return ans;
}

template<class K, class V> bool LockedHash<K,V>::pop(K k, V &v)
{
  bool ans;
  lock.wrlock();
  if (ans = !(m.find(k) == m.end())) {
    v= m[k];
    m.erase(k);
  }
  lock.unlock();
  return ans;
}

template<class K, class V> bool
LockedHash<K,V>::popif(K k, V &v, bool (*cond)(V v))
{
  bool ans;
  lock.wrlock();
  if (ans = (m.find(k) != m.end() && cond(m[k]))) {
    v = m[k];
    m.erase(k);
  }
  lock.unlock();
  return ans;
}

#endif // LOCKED_HASH_HPP
