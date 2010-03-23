# Collect environment vars, if they exist
HTTP_PROGNAME ?= ocotillo
STANDALONE_CACHE_PROGNAME ?= cash

BASE_SRCS = Locks.cpp Scheduler.cpp Server.cpp sigmasks.cpp Work.cpp Worker.cpp
BASE_OBJS = $(BASE_SRCS:.cpp=.o)

HTTP_SRCS = FileCache.cpp HTTP_cmdline.cpp HTTP_constants.cpp \
			HTTP_FindWork.cpp HTTP_main.cpp HTTP_Server.cpp \
			HTTP_Work.cpp inotifyFileCache.cpp
HTTP_OBJS = $(HTTP_SRCS:.cpp=.o)

STANDALONE_CACHE_SRCS = Cache_cmdline.cpp Cache_main.cpp CacheFindWork.cpp \
	CacheServer.cpp CacheWork.cpp FileCache.cpp inotifyFileCache.cpp 
STANDALONE_CACHE_OBJS = $(STANDALONE_CACHE_SRCS:.cpp=.o)

all: http standalone-cache
http: $(BASE_OBJS) $(HTTP_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -lpthread \
	$(BASE_OBJS) $(HTTP_OBJS) -o $(BIN_DIR)/$(HTTP_PROGNAME)
standalone-cache: $(BASE_OBJS) $(STANDALONE_CACHE_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -lpthread \
	$(BASE_OBJS) $(STANDALONE_CACHE_OBJS) \
	-o $(BIN_DIR)/$(STANDALONE_CACHE_PROGNAME)
.PHONY: depend
depend:
	makedepend -Y $(SRC_INC_DIRS:%=-I%) $(BASE_SRCS) $(HTTP_SRCS) \
	$(STANDALONE_CACHE_SRCS) 2> /dev/null
.PHONY: clean
clean:
	rm -f a.out *.o *~ *\#

# DO NOT DELETE

Locks.o: Locks.hpp logging.h
Scheduler.o: Scheduler.hpp Callback.hpp LockedQueue.hpp Locks.hpp logging.h
Scheduler.o: FindWork.hpp Work.hpp Workmap.hpp ServerErrs.hpp sigmasks.hpp
Server.o: logging.h Server.hpp FindWork.hpp Work.hpp Callback.hpp Workmap.hpp
Server.o: LockedQueue.hpp Locks.hpp Scheduler.hpp Thread.hpp Worker.hpp
Server.o: SigThread.hpp
sigmasks.o: logging.h sigmasks.hpp
Work.o: logging.h Work.hpp Callback.hpp
Worker.o: logging.h ServerErrs.hpp Worker.hpp LockedQueue.hpp Locks.hpp
Worker.o: Work.hpp Callback.hpp
FileCache.o: FileCache.hpp LockedQueue.hpp Locks.hpp logging.h FindWork.hpp
FileCache.o: Work.hpp Callback.hpp Workmap.hpp
HTTP_cmdline.o: HTTP_cmdline.hpp cmdline.hpp /home/brendan/http/data/sigs.def
HTTP_cmdline.o: /home/brendan/http/data/HTTP_cmdline.def
HTTP_constants.o: HTTP_constants.hpp /home/brendan/http/data/HTTP_status.def
HTTP_constants.o: /home/brendan/http/data/HTTP_methods.def
HTTP_constants.o: /home/brendan/http/data/HTTP_headers.def HTTP_Parse_Err.hpp
HTTP_constants.o: ServerErrs.hpp logging.h
HTTP_FindWork.o: HTTP_FindWork.hpp FileCache.hpp LockedQueue.hpp Locks.hpp
HTTP_FindWork.o: logging.h FindWork.hpp Work.hpp Callback.hpp Workmap.hpp
HTTP_FindWork.o: FindWork_prealloc.hpp HTTP_Work.hpp HTTP_constants.hpp
HTTP_FindWork.o: /home/brendan/http/data/HTTP_status.def
HTTP_FindWork.o: /home/brendan/http/data/HTTP_methods.def
HTTP_FindWork.o: /home/brendan/http/data/HTTP_headers.def Scheduler.hpp
HTTP_main.o: logging.h HTTP_cmdline.hpp cmdline.hpp
HTTP_main.o: /home/brendan/http/data/sigs.def
HTTP_main.o: /home/brendan/http/data/HTTP_cmdline.def HTTP_Server.hpp
HTTP_main.o: HTTP_constants.hpp /home/brendan/http/data/HTTP_status.def
HTTP_main.o: /home/brendan/http/data/HTTP_methods.def
HTTP_main.o: /home/brendan/http/data/HTTP_headers.def Workmap.hpp Work.hpp
HTTP_main.o: Callback.hpp HTTP_FindWork.hpp FileCache.hpp LockedQueue.hpp
HTTP_main.o: Locks.hpp FindWork.hpp FindWork_prealloc.hpp HTTP_Work.hpp
HTTP_main.o: Scheduler.hpp inotifyFileCache.hpp Server.hpp Thread.hpp
HTTP_main.o: Worker.hpp ServerErrs.hpp
HTTP_Server.o: HTTP_Server.hpp HTTP_cmdline.hpp cmdline.hpp
HTTP_Server.o: /home/brendan/http/data/sigs.def
HTTP_Server.o: /home/brendan/http/data/HTTP_cmdline.def HTTP_constants.hpp
HTTP_Server.o: /home/brendan/http/data/HTTP_status.def
HTTP_Server.o: /home/brendan/http/data/HTTP_methods.def
HTTP_Server.o: /home/brendan/http/data/HTTP_headers.def Workmap.hpp Work.hpp
HTTP_Server.o: Callback.hpp HTTP_FindWork.hpp FileCache.hpp LockedQueue.hpp
HTTP_Server.o: Locks.hpp logging.h FindWork.hpp FindWork_prealloc.hpp
HTTP_Server.o: HTTP_Work.hpp Scheduler.hpp inotifyFileCache.hpp Server.hpp
HTTP_Server.o: Thread.hpp Worker.hpp ServerErrs.hpp SigThread.hpp
HTTP_Work.o: HTTP_cmdline.hpp cmdline.hpp /home/brendan/http/data/sigs.def
HTTP_Work.o: /home/brendan/http/data/HTTP_cmdline.def HTTP_Parse_Err.hpp
HTTP_Work.o: HTTP_constants.hpp /home/brendan/http/data/HTTP_status.def
HTTP_Work.o: /home/brendan/http/data/HTTP_methods.def
HTTP_Work.o: /home/brendan/http/data/HTTP_headers.def ServerErrs.hpp
HTTP_Work.o: HTTP_Work.hpp FileCache.hpp LockedQueue.hpp Locks.hpp logging.h
HTTP_Work.o: FindWork.hpp Work.hpp Callback.hpp Workmap.hpp
HTTP_Work.o: FindWork_prealloc.hpp Scheduler.hpp
inotifyFileCache.o: inotifyFileCache.hpp Callback.hpp FileCache.hpp
inotifyFileCache.o: LockedQueue.hpp Locks.hpp logging.h FindWork.hpp Work.hpp
inotifyFileCache.o: Workmap.hpp Scheduler.hpp
Cache_cmdline.o: Cache_cmdline.hpp cmdline.hpp
Cache_cmdline.o: /home/brendan/http/data/sigs.def
Cache_cmdline.o: /home/brendan/http/data/Cache_cmdline.def
Cache_main.o: Cache_cmdline.hpp cmdline.hpp /home/brendan/http/data/sigs.def
Cache_main.o: /home/brendan/http/data/Cache_cmdline.def CacheServer.hpp
Cache_main.o: CacheFindWork.hpp CacheWork.hpp FileCache.hpp LockedQueue.hpp
Cache_main.o: Locks.hpp logging.h FindWork.hpp Work.hpp Callback.hpp
Cache_main.o: Workmap.hpp FindWork_prealloc.hpp Scheduler.hpp ServerErrs.hpp
Cache_main.o: inotifyFileCache.hpp Server.hpp Thread.hpp Worker.hpp
CacheFindWork.o: CacheFindWork.hpp CacheWork.hpp FileCache.hpp
CacheFindWork.o: LockedQueue.hpp Locks.hpp logging.h FindWork.hpp Work.hpp
CacheFindWork.o: Callback.hpp Workmap.hpp FindWork_prealloc.hpp Scheduler.hpp
CacheFindWork.o: ServerErrs.hpp
CacheServer.o: CacheServer.hpp CacheFindWork.hpp CacheWork.hpp FileCache.hpp
CacheServer.o: LockedQueue.hpp Locks.hpp logging.h FindWork.hpp Work.hpp
CacheServer.o: Callback.hpp Workmap.hpp FindWork_prealloc.hpp Scheduler.hpp
CacheServer.o: ServerErrs.hpp inotifyFileCache.hpp Server.hpp Thread.hpp
CacheServer.o: Worker.hpp
CacheWork.o: CacheWork.hpp FileCache.hpp LockedQueue.hpp Locks.hpp logging.h
CacheWork.o: FindWork.hpp Work.hpp Callback.hpp Workmap.hpp
CacheWork.o: FindWork_prealloc.hpp Scheduler.hpp ServerErrs.hpp
FileCache.o: FileCache.hpp LockedQueue.hpp Locks.hpp logging.h FindWork.hpp
FileCache.o: Work.hpp Callback.hpp Workmap.hpp
inotifyFileCache.o: inotifyFileCache.hpp Callback.hpp FileCache.hpp
inotifyFileCache.o: LockedQueue.hpp Locks.hpp logging.h FindWork.hpp Work.hpp
inotifyFileCache.o: Workmap.hpp Scheduler.hpp
