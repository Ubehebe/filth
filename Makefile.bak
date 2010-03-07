HTTP_PROGNAME ?= ocotillo
CGI_PROGNAME ?= ollitoco

BASE_SRCS = Scheduler.cpp Server.cpp Worker.cpp Locks.cpp sigmasks.cpp
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

Scheduler.o: Scheduler.hpp LockedQueue.hpp Locks.hpp Work.hpp ServerErrs.hpp
Server.o: Server.hpp LockedQueue.hpp Locks.hpp Scheduler.hpp Work.hpp
Server.o: Thread.hpp sigmasks.hpp Thread.cpp Worker.hpp
Worker.o: Worker.hpp LockedQueue.hpp Locks.hpp Work.hpp
Locks.o: Locks.hpp
sigmasks.o: sigmasks.hpp
HTTP_constants.o: HTTP_constants.hpp HTTP_status.def HTTP_methods.def
HTTP_constants.o: HTTP_headers.def HTTP_Parse_Err.hpp ServerErrs.hpp
HTTP_cmdline.o: HTTP_cmdline.hpp HTTP_cmdline.def
HTTP_main.o: HTTP_Server.hpp HTTP_cmdline.hpp HTTP_cmdline.def HTTP_Work.hpp
HTTP_main.o: LockedQueue.hpp Locks.hpp Scheduler.hpp Work.hpp Server.hpp
HTTP_main.o: Thread.hpp sigmasks.hpp Thread.cpp Worker.hpp ServerErrs.hpp
HTTP_Work.o: HTTP_Work.hpp LockedQueue.hpp Locks.hpp Scheduler.hpp Work.hpp
