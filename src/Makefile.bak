# Collect environment vars, if they exist
HTTP_PROGNAME ?= ocotillo
STANDALONE_CACHE_PROGNAME ?= cash

BASE_SRCS = Locks.cpp Scheduler.cpp Server.cpp sigmasks.cpp Work.cpp Worker.cpp
BASE_OBJS = $(BASE_SRCS:.cpp=.o)

HTTP_SRCS = FileCache.cpp HTTP_cmdline.cpp HTTP_constants.cpp \
			HTTP_FindWork.cpp HTTP_main.cpp HTTP_Server.cpp \
			HTTP_Work.cpp inotifyFileCache.cpp
HTTP_OBJS = $(HTTP_SRCS:.cpp=.o)

CONCURRENT_QUEUE_TEST_SRCS = ConcurrentQueueTest.cpp Locks.cpp
CONCURRENT_QUEUE_TEST_OBJS = $(CONCURRENT_QUEUE_TEST_SRCS:.cpp=.o)

ENDLESS_TEST_SRCS = EndlessTest.c
ENDLESS_TEST_OBJS = $(ENDLESS_TEST_SRCS:.c=.o)

THREAD_POOL_TEST_SRCS = ThreadPoolTest.cpp Locks.cpp
THREAD_POOL_TEST_OBJS = $(THREAD_POOL_TEST_SRCS:.cpp=.o)

STANDALONE_CACHE_SRCS = Cache_cmdline.cpp Cache_main.cpp CacheFindWork.cpp \
	CacheServer.cpp CacheWork.cpp FileCache.cpp inotifyFileCache.cpp 
STANDALONE_CACHE_OBJS = $(STANDALONE_CACHE_SRCS:.cpp=.o)

all: http concurrent-queue-test endless-test
http: $(BASE_OBJS) $(HTTP_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -lpthread \
	$(BASE_OBJS) $(HTTP_OBJS) -o $(BIN_DIR)/$(HTTP_PROGNAME)
standalone-cache: $(BASE_OBJS) $(STANDALONE_CACHE_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -lpthread \
	$(BASE_OBJS) $(STANDALONE_CACHE_OBJS) \
	-o $(BIN_DIR)/$(STANDALONE_CACHE_PROGNAME)
concurrent-queue-test: $(CONCURRENT_QUEUE_TEST_OBJS) 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -lpthread \
	$(CONCURRENT_QUEUE_TEST_OBJS) -o $(BIN_DIR)/concurrent-queue-test
endless-test: $(ENDLESS_TEST_OBJS)
	$(CC) $(ENDLESS_TEST_OBJS) -o $(BIN_DIR)/endless-test
SigThreadTest: $(SIGTHREAD_TEST_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -lpthread \
	$(SIGTHREAD_TEST_OBJS) -o $(BIN_DIR)/SigThreadTest
ThreadPoolTest: $(THREAD_POOL_TEST_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -lpthread \
	$(THREAD_POOL_TEST_OBJS) -o $(BIN_DIR)/ThreadPoolTest


.PHONY: depend
depend:
	makedepend -Y $(SRC_INC_DIRS:%=-I%) $(BASE_SRCS) $(HTTP_SRCS) \
	$(CONCURRENT_QUEUE_TEST_SRCS) $(STANDALONE_CACHE_SRCS) \
	$(ENDLESS_TEST_SRCS) $(SIGTHREAD_TEST_SRCS) $(THREAD_POOL_TEST_SRCS) \
	2> /dev/null
.PHONY: clean
clean:
	rm -f a.out *.o *~ *\#

# DO NOT DELETE

Locks.o: Locks.hpp logging.h
Scheduler.o: Scheduler.hpp Callback.hpp ConcurrentQueue.hpp logging.h
Scheduler.o: Factory.hpp FindWork.hpp Work.hpp Workmap.hpp ServerErrs.hpp
Scheduler.o: sigmasks.hpp
Server.o: Factory.hpp logging.h Server.hpp Callback.hpp FindWork.hpp Work.hpp
Server.o: Workmap.hpp DoubleLockedQueue.hpp ConcurrentQueue.hpp Locks.hpp
Server.o: Scheduler.hpp Worker.hpp sigmasks.hpp ThreadPool.hpp Thread.hpp
sigmasks.o: logging.h sigmasks.hpp
Work.o: logging.h Work.hpp Callback.hpp
Worker.o: logging.h ServerErrs.hpp Worker.hpp ConcurrentQueue.hpp Factory.hpp
Worker.o: Work.hpp Callback.hpp
FileCache.o: FileCache.hpp DoubleLockedQueue.hpp ConcurrentQueue.hpp
FileCache.o: Locks.hpp logging.h FindWork.hpp Work.hpp Callback.hpp
FileCache.o: Workmap.hpp
HTTP_cmdline.o: HTTP_cmdline.hpp cmdline.hpp /home/brendan/http/data/sigs.def
HTTP_cmdline.o: /home/brendan/http/data/HTTP_cmdline.def
HTTP_constants.o: HTTP_constants.hpp /home/brendan/http/data/HTTP_status.def
HTTP_constants.o: /home/brendan/http/data/HTTP_methods.def
HTTP_constants.o: /home/brendan/http/data/HTTP_headers.def HTTP_Parse_Err.hpp
HTTP_constants.o: ServerErrs.hpp logging.h
HTTP_FindWork.o: HTTP_FindWork.hpp FileCache.hpp DoubleLockedQueue.hpp
HTTP_FindWork.o: ConcurrentQueue.hpp Locks.hpp logging.h FindWork.hpp
HTTP_FindWork.o: Work.hpp Callback.hpp Workmap.hpp FindWork_prealloc.hpp
HTTP_FindWork.o: HTTP_Work.hpp HTTP_constants.hpp
HTTP_FindWork.o: /home/brendan/http/data/HTTP_status.def
HTTP_FindWork.o: /home/brendan/http/data/HTTP_methods.def
HTTP_FindWork.o: /home/brendan/http/data/HTTP_headers.def Scheduler.hpp
HTTP_FindWork.o: Factory.hpp
HTTP_main.o: logging.h HTTP_cmdline.hpp cmdline.hpp
HTTP_main.o: /home/brendan/http/data/sigs.def
HTTP_main.o: /home/brendan/http/data/HTTP_cmdline.def HTTP_Server.hpp
HTTP_main.o: Callback.hpp HTTP_constants.hpp
HTTP_main.o: /home/brendan/http/data/HTTP_status.def
HTTP_main.o: /home/brendan/http/data/HTTP_methods.def
HTTP_main.o: /home/brendan/http/data/HTTP_headers.def Workmap.hpp Work.hpp
HTTP_main.o: HTTP_FindWork.hpp FileCache.hpp DoubleLockedQueue.hpp
HTTP_main.o: ConcurrentQueue.hpp Locks.hpp FindWork.hpp FindWork_prealloc.hpp
HTTP_main.o: HTTP_Work.hpp Scheduler.hpp Factory.hpp inotifyFileCache.hpp
HTTP_main.o: Server.hpp Worker.hpp ServerErrs.hpp
HTTP_Server.o: HTTP_Server.hpp Callback.hpp HTTP_cmdline.hpp cmdline.hpp
HTTP_Server.o: /home/brendan/http/data/sigs.def
HTTP_Server.o: /home/brendan/http/data/HTTP_cmdline.def HTTP_constants.hpp
HTTP_Server.o: /home/brendan/http/data/HTTP_status.def
HTTP_Server.o: /home/brendan/http/data/HTTP_methods.def
HTTP_Server.o: /home/brendan/http/data/HTTP_headers.def Workmap.hpp Work.hpp
HTTP_Server.o: HTTP_FindWork.hpp FileCache.hpp DoubleLockedQueue.hpp
HTTP_Server.o: ConcurrentQueue.hpp Locks.hpp logging.h FindWork.hpp
HTTP_Server.o: FindWork_prealloc.hpp HTTP_Work.hpp Scheduler.hpp Factory.hpp
HTTP_Server.o: inotifyFileCache.hpp Server.hpp Worker.hpp ServerErrs.hpp
HTTP_Server.o: ThreadPool.hpp Thread.hpp
HTTP_Work.o: HTTP_cmdline.hpp cmdline.hpp /home/brendan/http/data/sigs.def
HTTP_Work.o: /home/brendan/http/data/HTTP_cmdline.def HTTP_Parse_Err.hpp
HTTP_Work.o: HTTP_constants.hpp /home/brendan/http/data/HTTP_status.def
HTTP_Work.o: /home/brendan/http/data/HTTP_methods.def
HTTP_Work.o: /home/brendan/http/data/HTTP_headers.def ServerErrs.hpp
HTTP_Work.o: HTTP_Work.hpp FileCache.hpp DoubleLockedQueue.hpp
HTTP_Work.o: ConcurrentQueue.hpp Locks.hpp logging.h FindWork.hpp Work.hpp
HTTP_Work.o: Callback.hpp Workmap.hpp FindWork_prealloc.hpp Scheduler.hpp
HTTP_Work.o: Factory.hpp
inotifyFileCache.o: inotifyFileCache.hpp Callback.hpp FileCache.hpp
inotifyFileCache.o: DoubleLockedQueue.hpp ConcurrentQueue.hpp Locks.hpp
inotifyFileCache.o: logging.h FindWork.hpp Work.hpp Workmap.hpp Scheduler.hpp
inotifyFileCache.o: Factory.hpp
ConcurrentQueueTest.o: Factory.hpp Locks.hpp logging.h DoubleLockedQueue.hpp
ConcurrentQueueTest.o: ConcurrentQueue.hpp ThreadPool.hpp Callback.hpp
ConcurrentQueueTest.o: Thread.hpp
Locks.o: Locks.hpp logging.h
Cache_cmdline.o: Cache_cmdline.hpp cmdline.hpp
Cache_cmdline.o: /home/brendan/http/data/sigs.def
Cache_cmdline.o: /home/brendan/http/data/Cache_cmdline.def
Cache_main.o: Cache_cmdline.hpp cmdline.hpp /home/brendan/http/data/sigs.def
Cache_main.o: /home/brendan/http/data/Cache_cmdline.def CacheServer.hpp
Cache_main.o: CacheFindWork.hpp CacheWork.hpp FileCache.hpp
Cache_main.o: DoubleLockedQueue.hpp ConcurrentQueue.hpp Locks.hpp logging.h
Cache_main.o: FindWork.hpp Work.hpp Callback.hpp Workmap.hpp
Cache_main.o: FindWork_prealloc.hpp Scheduler.hpp Factory.hpp ServerErrs.hpp
Cache_main.o: inotifyFileCache.hpp Server.hpp Worker.hpp
CacheFindWork.o: CacheFindWork.hpp CacheWork.hpp FileCache.hpp
CacheFindWork.o: DoubleLockedQueue.hpp ConcurrentQueue.hpp Locks.hpp
CacheFindWork.o: logging.h FindWork.hpp Work.hpp Callback.hpp Workmap.hpp
CacheFindWork.o: FindWork_prealloc.hpp Scheduler.hpp Factory.hpp
CacheFindWork.o: ServerErrs.hpp
CacheServer.o: CacheServer.hpp CacheFindWork.hpp CacheWork.hpp FileCache.hpp
CacheServer.o: DoubleLockedQueue.hpp ConcurrentQueue.hpp Locks.hpp logging.h
CacheServer.o: FindWork.hpp Work.hpp Callback.hpp Workmap.hpp
CacheServer.o: FindWork_prealloc.hpp Scheduler.hpp Factory.hpp ServerErrs.hpp
CacheServer.o: inotifyFileCache.hpp Server.hpp Worker.hpp
CacheWork.o: CacheWork.hpp FileCache.hpp DoubleLockedQueue.hpp
CacheWork.o: ConcurrentQueue.hpp Locks.hpp logging.h FindWork.hpp Work.hpp
CacheWork.o: Callback.hpp Workmap.hpp FindWork_prealloc.hpp Scheduler.hpp
CacheWork.o: Factory.hpp ServerErrs.hpp
FileCache.o: FileCache.hpp DoubleLockedQueue.hpp ConcurrentQueue.hpp
FileCache.o: Locks.hpp logging.h FindWork.hpp Work.hpp Callback.hpp
FileCache.o: Workmap.hpp
inotifyFileCache.o: inotifyFileCache.hpp Callback.hpp FileCache.hpp
inotifyFileCache.o: DoubleLockedQueue.hpp ConcurrentQueue.hpp Locks.hpp
inotifyFileCache.o: logging.h FindWork.hpp Work.hpp Workmap.hpp Scheduler.hpp
inotifyFileCache.o: Factory.hpp
ThreadPoolTest.o: Factory.hpp Locks.hpp ThreadPool.hpp Callback.hpp
ThreadPoolTest.o: Thread.hpp logging.h
Locks.o: Locks.hpp logging.h
