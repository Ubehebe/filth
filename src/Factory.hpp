#ifndef FACTORY_HPP
#define FACTORY_HPP

// This is not all that flexible because T has to be default constructible.
template<class T> class Factory
{
public:
  T *operator()() { return new T(); }
};

#endif // FACTORY_HPP
