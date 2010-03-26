#ifndef FACTORY_HPP
#define FACTORY_HPP

/* An object that knows how to make (or find) T's.
 * Instead of inheriting, you write a template specialization, thus:
 * template<> class Factory<foo> { ... }
 *
 * TODO: my "FindWork" class is basically a factory that takes additional
 * arguments. Couldn't we make this into a factory somehow? */
template<class T> class Factory
{
public:
  T *operator()();
};

#endif // FACTORY_HPP
