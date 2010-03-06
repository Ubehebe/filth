HTTP_PROGNAME ?= ocotillo
CGI_PROGNAME ?= ollitoco

BASE_SRCS = Scheduler_new.cpp Server_new.cpp Worker_new.cpp Locks.cpp
BASE_OBJS = $(BASE_SRCS:.cpp=.o)

HTTP_SRCS = HTTP_constants.cpp HTTP_cmdline.cpp HTTP_main.cpp HTTP_Work.cpp
HTTP_OBJS = $(HTTP_SRCS:.cpp=.o)

CGI_SRCS = CGI_main.cpp
CGI_OBJS = $(CGI_SRCS:.cpp=.o)

all: http cgi
http: $(BASE_OBJS) $(HTTP_OBJS)
	$(CXX) -lpthread $(BASE_OBJS) $(HTTP_OBJS) -o $(HTTP_PROGNAME)
cgi: $(BASE_OBJS) $(CGI_OBJS)
	$(CXX) -lpthread $(BASE_OBJS) $(CGI_OBJS) -o $(CGI_PROGNAME)
depend:
	makedepend -Y $(BASE_SRCS) $(HTTP_SRCS) $(UNIX_SRCS) 2> /dev/null
clean:
	rm -f *.o *~ *\# $(HTTP_PROGNAME)
# DO NOT DELETE

Scheduler_new.o: Scheduler_new.hpp LockedQueue.hpp Locks.hpp Work.hpp
Scheduler_new.o: ServerErrs.hpp
Server_new.o: Server_new.hpp LockedQueue.hpp Locks.hpp Scheduler_new.hpp
Server_new.o: Work.hpp Thread.hpp Thread.cpp Worker_new.hpp
Worker_new.o: Worker_new.hpp LockedQueue.hpp Locks.hpp Work.hpp
Locks.o: Locks.hpp
HTTP_constants.o: HTTP_constants.hpp HTTP_status.def HTTP_methods.def
HTTP_constants.o: HTTP_headers.def HTTP_Parse_Err.hpp ServerErrs.hpp
HTTP_cmdline.o: HTTP_cmdline.hpp HTTP_cmdline.def
HTTP_main.o: HTTP_Server.hpp HTTP_cmdline.hpp HTTP_cmdline.def HTTP_Work.hpp
HTTP_main.o: LockedQueue.hpp Locks.hpp Scheduler_new.hpp Work.hpp
HTTP_main.o: Server_new.hpp Thread.hpp Thread.cpp Worker_new.hpp
HTTP_main.o: ServerErrs.hpp
HTTP_Work.o: HTTP_Work.hpp LockedQueue.hpp Locks.hpp Scheduler_new.hpp
HTTP_Work.o: Work.hpp
