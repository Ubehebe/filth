#ifndef CALLBACK_HPP
#define CALLBACK_HPP

/** \brief Base class for objects that you can call with no arguments.
 * \remarks Obviously, it's easy to give any class a nullary operator().
 * Deriving from this class says that you intend to use the operator()
 * specifically as a callback. */
class Callback
{
public:
  /** \brief The callback. */
  virtual void operator()() = 0;
  virtual ~Callback() {}
};

#endif // CALLBACK_HPP
