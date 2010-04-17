#ifndef ROOT_SAFETY_HPP
#define ROOT_SAFETY_HPP

class root_safety
{
public:
  static void root_giveup(uid_t untrusted);
  static void root_getback();
  static void untrusted_sanity_checks();
private:
  root_safety();
  static uid_t const DANGER_root = 0;
};

#endif // ROOT_SAFETY_HPP
