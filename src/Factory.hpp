#ifndef FACTORY_HPP
#define FACTORY_HPP

/* An object that knows how to make T's.
 * Instead of inheriting, you write a template specialization, thus:
 * template<> class Factory<foo> { ... } */
template<class T> class Factory
{
public:
  T *operator()();
};

#endif // FACTORY_HPP
