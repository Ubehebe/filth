#ifndef ROOT_SAFETY_HPP
#define ROOT_SAFETY_HPP

/** \brief Giving up and getting back superuser privileges. */
class root_safety
{
public:
  /** \brief Switch from superuser to given untrusted user and group.
   * Also performs some safety checks after we've successfully given up
   * superuser privileges; for example, the euid/egid we switch to
   * shouldn't be the owner of the current directory.
   * \param untrusted_uid uid to switch to
   * \param untrusted_gid gid to switch to
   * \warning Will terminate the process if the seteuid, setegid, or any of the
   * sanity checks fails. A terminated process is better than one that could be
   * doing something scary. */
  static void root_giveup(uid_t untrusted_uid, gid_t untrusted_gid);
  /** \brief Switch from untrusted user to superuser.
   * This probably means the process had to have been started with superuser
   * privileges.
   * \warning If the switch fails, the process is terminated. */
  static void root_getback();

private:
  root_safety();
  static uid_t const DANGER_root = 0;
  static void untrusted_sanity_checks();
};

#endif // ROOT_SAFETY_HPP
