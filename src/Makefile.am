bin_PROGRAMS=ess-a-byte \
	ess-a-byte-debug \
	concurrent-queue-test \
	concurrent-queue-endless-test \
	thread-pool-test

BASE=Locks.cpp Scheduler.cpp Server.cpp sigmasks.cpp Work.cpp Worker.cpp
ess_a_byte_SOURCES = $(BASE) FileCache.cpp HTTP_cmdline.cpp HTTP_constants.cpp \
	HTTP_FindWork.cpp HTTP_main.cpp HTTP_Server.cpp HTTP_Work.cpp \
	inotifyFileCache.cpp

ess_a_byte_debug_SOURCES = $(ess_a_byte_SOURCES)
ess_a_byte_debug_CPPFLAGS = -D _LOG_DEBUG

concurrent_queue_test_SOURCES = ConcurrentQueueTest.cpp Locks.cpp

concurrent_queue_endless_test_SOURCES = endless_test.c

thread_pool_test_SOURCES = Locks.cpp ThreadPoolTest.cpp