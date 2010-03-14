/* More amusing than the syntax error (which is fairly obvious) is the non-
 * obvious behavior it causes. If we subsequently perform a blocking read
 * from fd, it won't return an error, but rather will appear to block forever.
 * This of course is because fd is now probably 0, which is probably standard
 * input, which of course is going to block on a read! */
if (fd = open(path.c_str(), O_RDONLY) ==-1) {
  _LOG_DEBUG("open %s: %m", path.c_str());
  return NULL;
 }
