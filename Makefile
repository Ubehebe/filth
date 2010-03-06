HTTP_PROGNAME ?= ocotillo
CGI_PROGNAME ?= ollitoco

BASE_SRCS = Scheduler_new.cpp Server_new.cpp Worker_new.cpp Locks.cpp
BASE_OBJS = $(BASE_SRCS:.cpp=.o)

HTTP_SRCS = HTTP_constants.cpp HTTP_env.cpp HTTP_main.cpp HTTP_Work.cpp
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

Scheduler_new.o: Scheduler_new.h LockedQueue.h Locks.h Work.h ServerErrs.h
Server_new.o: Server_new.h LockedQueue.h Locks.h Scheduler_new.h Work.h
Server_new.o: Thread.h Thread.cpp Worker_new.h
Worker_new.o: Worker_new.h LockedQueue.h Locks.h Work.h
Locks.o: Locks.h
HTTP_constants.o: HTTP_constants.h HTTP_status.def HTTP_methods.def
HTTP_constants.o: HTTP_headers.def HTTP_Parse_Err.h ServerErrs.h
HTTP_env.o: HTTP_env.h HTTP_env.def
HTTP_main.o: HTTP_Server.h HTTP_env.h HTTP_env.def HTTP_Work.h LockedQueue.h
HTTP_main.o: Locks.h Scheduler_new.h Work.h Server_new.h Thread.h Thread.cpp
HTTP_main.o: Worker_new.h ServerErrs.h
HTTP_Work.o: HTTP_Work.h LockedQueue.h Locks.h Scheduler_new.h Work.h
