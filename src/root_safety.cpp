#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "logging.h"
#include "root_safety.hpp"

void root_safety::root_giveup(uid_t untrusted_uid, gid_t untrusted_gid)
{
  if (setegid(untrusted_gid)==-1) {
    _LOG_FATAL("setegid %d: %m", untrusted_gid);
    exit(1);
  }
  if (seteuid(untrusted_uid)==-1) {
    _LOG_FATAL("seteuid %d: %m", untrusted_uid);
    exit(1);
  }
  untrusted_sanity_checks();
}

void root_safety::root_getback()
{
  if (seteuid(DANGER_root)==-1) {
    _LOG_FATAL("seteuid: %d %m", DANGER_root);
    exit(1);
  }
}

void root_safety::untrusted_sanity_checks()
{
  struct stat st;
  if (stat(".", &st) == -1) {
    _LOG_FATAL("stat .: %m");
    exit(1);
  }
  if (geteuid() == DANGER_root) {
    _LOG_FATAL("you wanted to perform untrusted sanity checks "
	       "but you're still the superuser, exiting");
    exit(1);
  }
  else if (st.st_uid == geteuid()) {
    _LOG_FATAL("euid %d is also the owner of the current directory. "
	       "seems like a bad idea for an untrusted program, exiting",
	       st.st_uid);
    exit(1);
  }
  else if (st.st_gid == getegid()) {
    _LOG_FATAL("egid %d is also the owner of the current directory. "
	       "seems like a bad idea for an untrusted program, exiting",
	       st.st_gid);
    exit(1);
  }
  else if (st.st_mode & S_IWOTH) {
    _LOG_FATAL("current directory is writable by others. "
	       "seems like a bad idea for an untrusted program, exiting");
    exit(1);
  }
}
