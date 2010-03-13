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
# change this to e.g. _LOG_NOTICE later.
CPPFLAGS = -D _LOG_DEBUG

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

Scheduler.o: logging.h Scheduler.hpp LockedQueue.hpp Locks.hpp Work.hpp
Scheduler.o: ServerErrs.hpp sigmasks.hpp
Server.o: Server.hpp LockedQueue.hpp Locks.hpp Scheduler.hpp Work.hpp
Server.o: Thread.hpp sigmasks.hpp Thread.cpp logging.h Worker.hpp
Worker.o: logging.h sigmasks.hpp Worker.hpp LockedQueue.hpp Locks.hpp
Worker.o: Work.hpp
Locks.o: Locks.hpp logging.h
sigmasks.o: logging.h sigmasks.hpp
FileCache.o: FileCache.hpp LockedQueue.hpp Locks.hpp logging.h
HTTP_constants.o: HTTP_constants.hpp HTTP_status.def HTTP_methods.def
HTTP_constants.o: HTTP_headers.def HTTP_Parse_Err.hpp ServerErrs.hpp
HTTP_cmdline.o: HTTP_cmdline.hpp HTTP_cmdline.def
HTTP_main.o: logging.h HTTP_Server.hpp FileCache.hpp LockedQueue.hpp
HTTP_main.o: Locks.hpp HTTP_cmdline.hpp HTTP_cmdline.def HTTP_Statemap.hpp
HTTP_main.o: Work.hpp HTTP_Work.hpp HTTP_constants.hpp HTTP_status.def
HTTP_main.o: HTTP_methods.def HTTP_headers.def Scheduler.hpp Server.hpp
HTTP_main.o: Thread.hpp sigmasks.hpp Thread.cpp Worker.hpp ServerErrs.hpp
HTTP_Work.o: HTTP_cmdline.hpp HTTP_cmdline.def HTTP_Parse_Err.hpp
HTTP_Work.o: HTTP_constants.hpp HTTP_status.def HTTP_methods.def
HTTP_Work.o: HTTP_headers.def ServerErrs.hpp HTTP_Work.hpp FileCache.hpp
HTTP_Work.o: LockedQueue.hpp Locks.hpp HTTP_Statemap.hpp Work.hpp
HTTP_Work.o: Scheduler.hpp logging.h
