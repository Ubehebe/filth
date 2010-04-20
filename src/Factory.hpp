#ifndef FACTORY_HPP
#define FACTORY_HPP

/** \brief Makes instances of another class via default constructor. */
template<class T> class Factory
{
public:
  /** \brief just call the default constructor. */
  T *operator()() { return new T(); }
};

#endif // FACTORY_HPP
