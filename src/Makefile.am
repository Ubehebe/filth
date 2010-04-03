bin_PROGRAMS=ess-a-byte \
	concurrent-queue-test \
	concurrent-queue-endless-test \
	thread-pool-test \
	standalone-cache

# Just so we can say e.g. "make LOG=_LOG_DEBUG" on the command line.
LOG ?= _LOG_FATAL
AM_CPPFLAGS = -D $(LOG) -D _COLLECT_STATS

BASE=Locks.cpp Scheduler.cpp Server.cpp sigmasks.cpp Work.cpp Worker.cpp \
	Time_nonthreadsafe.cpp
ess_a_byte_SOURCES = $(BASE) FileCache.cpp HTTP_cmdline.cpp HTTP_constants.cpp \
	HTTP_FindWork.cpp HTTP_main.cpp HTTP_Server.cpp HTTP_Work.cpp \
	inotifyFileCache.cpp Magic.cpp MIME_FileCache.cpp

concurrent_queue_test_SOURCES = ConcurrentQueueTest.cpp Locks.cpp

concurrent_queue_endless_test_SOURCES = endless_test.c

thread_pool_test_SOURCES = Locks.cpp ThreadPoolTest.cpp

standalone_cache_SOURCES = $(BASE) Cache_cmdline.cpp Cache_main.cpp \
	CacheFindWork.cpp CacheServer.cpp CacheWork.cpp FileCache.cpp \
	inotifyFileCache.cpp Magic.cpp MIME_FileCache.cpp