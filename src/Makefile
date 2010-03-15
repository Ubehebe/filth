# Collect environment vars, if they exist
HTTP_PROGNAME ?= ocotillo
CGI_PROGNAME ?= ollitoco

BASE_SRCS = Scheduler.cpp Server.cpp Worker.cpp Locks.cpp sigmasks.cpp
BASE_OBJS = $(BASE_SRCS:.cpp=.o)

HTTP_SRCS = FileCache.cpp HTTP_constants.cpp HTTP_cmdline.cpp HTTP_main.cpp \
			HTTP_Work.cpp
HTTP_OBJS = $(HTTP_SRCS:.cpp=.o)

CGI_SRCS = CGI_main.cpp
CGI_OBJS = $(CGI_SRCS:.cpp=.o)

CXXFLAGS = -std=c++0x
CPPFLAGS = -D _LOG_FATAL

all: http cgi
http: $(BASE_OBJS) $(HTTP_OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -lpthread $(BASE_OBJS) $(HTTP_OBJS) \
	-o $(HTTP_PROGNAME)
cgi: $(BASE_OBJS) $(CGI_OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -lpthread $(BASE_OBJS) $(CGI_OBJS) \
	-o $(CGI_PROGNAME)
depend:
	makedepend -Y $(BASE_SRCS) $(HTTP_SRCS) $(UNIX_SRCS) 2> /dev/null
clean:
	rm -f *.o *~ *\# a.out $(HTTP_PROGNAME) $(CGI_PROGNAME)
# DO NOT DELETE

Scheduler.o: Scheduler.hpp LockedQueue.hpp Locks.hpp Work.hpp ServerErrs.hpp
Scheduler.o: sigmasks.hpp
Server.o: Server.hpp LockedQueue.hpp Locks.hpp Scheduler.hpp Work.hpp
Server.o: Thread.hpp sigmasks.hpp Thread.cpp Worker.hpp
Worker.o: sigmasks.hpp Worker.hpp LockedQueue.hpp Locks.hpp Work.hpp
Locks.o: Locks.hpp
sigmasks.o: sigmasks.hpp
FileCache.o: FileCache.hpp LockedQueue.hpp Locks.hpp Scheduler.hpp Work.hpp
HTTP_constants.o: HTTP_constants.hpp ../data/HTTP_status.def
HTTP_constants.o: ../data/HTTP_methods.def ../data/HTTP_headers.def
HTTP_constants.o: HTTP_Parse_Err.hpp ServerErrs.hpp
HTTP_cmdline.o: HTTP_cmdline.hpp ../data/HTTP_cmdline.def
HTTP_main.o: HTTP_Server.hpp FileCache.hpp LockedQueue.hpp Locks.hpp
HTTP_main.o: Scheduler.hpp Work.hpp HTTP_cmdline.hpp ../data/HTTP_cmdline.def
HTTP_main.o: HTTP_constants.hpp ../data/HTTP_status.def
HTTP_main.o: ../data/HTTP_methods.def ../data/HTTP_headers.def
HTTP_main.o: HTTP_Statemap.hpp HTTP_Work.hpp Server.hpp Thread.hpp
HTTP_main.o: sigmasks.hpp Thread.cpp Worker.hpp ServerErrs.hpp
HTTP_Work.o: HTTP_cmdline.hpp ../data/HTTP_cmdline.def HTTP_Parse_Err.hpp
HTTP_Work.o: HTTP_constants.hpp ../data/HTTP_status.def
HTTP_Work.o: ../data/HTTP_methods.def ../data/HTTP_headers.def ServerErrs.hpp
HTTP_Work.o: HTTP_Work.hpp FileCache.hpp LockedQueue.hpp Locks.hpp
HTTP_Work.o: Scheduler.hpp Work.hpp HTTP_Statemap.hpp
