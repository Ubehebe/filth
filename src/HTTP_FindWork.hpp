#ifndef HTTP_FINDWORK_HPP
#define HTTP_FINDWORK_HPP

class HTTP_FindWork : public FindWork
{
public:
  HTTP_FindWork() {}
  ~HTTP_FindWork();
  Work *operator()(int fd, Work::mode m)
  {
    Workmap::iterator it;
  }
private:
  HTTP_FindWork(HTTP_FindWork const &);
  HTTP_FindWork &operator=(HTTP_FindWork const &);
};

#endif // HTTP_FINDWORK_HPP
