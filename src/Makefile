# Collect environment vars, if they exist
HTTP_PROGNAME ?= ocotillo
CGI_PROGNAME ?= ollitoco

BASE_SRCS = Scheduler.cpp Server.cpp Worker.cpp Locks.cpp sigmasks.cpp
BASE_OBJS = $(BASE_SRCS:.cpp=.o)

HTTP_SRCS = FileCache.cpp HTTP_constants.cpp HTTP_cmdline.cpp HTTP_main.cpp \
			HTTP_Work.cpp inotifyFileCache.cpp
HTTP_OBJS = $(HTTP_SRCS:.cpp=.o)

CGI_SRCS = CGI_main.cpp
CGI_OBJS = $(CGI_SRCS:.cpp=.o)

http: $(BASE_OBJS) $(HTTP_OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -lpthread $(BASE_OBJS) $(HTTP_OBJS) \
	-o $(HTTP_PROGNAME)
cgi: $(BASE_OBJS) $(CGI_OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -lpthread $(BASE_OBJS) $(CGI_OBJS) \
	-o $(CGI_PROGNAME)
.PHONY: depend
depend:
	makedepend $(INCLUDE_DIRS) -Y $(BASE_SRCS) $(HTTP_SRCS) 2> /dev/null
.PHONY: clean
clean:
	rm -f *.o *~ *\# a.out $(HTTP_PROGNAME) $(CGI_PROGNAME)
# DO NOT DELETE

Scheduler.o: logging.h Scheduler.hpp Callback.hpp LockedQueue.hpp Locks.hpp
Scheduler.o: FindWork.hpp Work.hpp ServerErrs.hpp sigmasks.hpp
Server.o: Server.hpp FindWork.hpp Work.hpp Callback.hpp LockedQueue.hpp
Server.o: Locks.hpp Scheduler.hpp Thread.hpp sigmasks.hpp Thread.cpp
Server.o: logging.h Worker.hpp
Worker.o: logging.h sigmasks.hpp Worker.hpp LockedQueue.hpp Locks.hpp
Worker.o: Work.hpp Callback.hpp
Locks.o: Locks.hpp logging.h
sigmasks.o: logging.h sigmasks.hpp
FileCache.o: FileCache.hpp LockedQueue.hpp Locks.hpp FindWork.hpp Work.hpp
FileCache.o: Callback.hpp logging.h
HTTP_constants.o: HTTP_constants.hpp HTTP_Parse_Err.hpp ServerErrs.hpp
HTTP_constants.o: logging.h
HTTP_cmdline.o: HTTP_cmdline.hpp
HTTP_main.o: logging.h HTTP_Server.hpp HTTP_cmdline.hpp HTTP_constants.hpp
HTTP_main.o: HTTP_Statemap.hpp Work.hpp Callback.hpp HTTP_FindWork.hpp
HTTP_main.o: FileCache.hpp LockedQueue.hpp Locks.hpp FindWork.hpp
HTTP_main.o: HTTP_Work.hpp Scheduler.hpp inotifyFileCache.hpp Server.hpp
HTTP_main.o: Thread.hpp sigmasks.hpp Thread.cpp Worker.hpp ServerErrs.hpp
HTTP_Work.o: HTTP_cmdline.hpp HTTP_Parse_Err.hpp HTTP_constants.hpp
HTTP_Work.o: ServerErrs.hpp HTTP_Work.hpp FileCache.hpp LockedQueue.hpp
HTTP_Work.o: Locks.hpp FindWork.hpp Work.hpp Callback.hpp HTTP_Statemap.hpp
HTTP_Work.o: Scheduler.hpp logging.h
inotifyFileCache.o: inotifyFileCache.hpp Callback.hpp FileCache.hpp
inotifyFileCache.o: LockedQueue.hpp Locks.hpp FindWork.hpp Work.hpp
inotifyFileCache.o: Scheduler.hpp logging.h
