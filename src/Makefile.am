bin_PROGRAMS=filth cgi-test-app

# Just so we can say e.g. "make LOG=_LOG_DEBUG" on the command line.
LOG ?= _LOG_FATAL
AM_CPPFLAGS = -D $(LOG) -D _COLLECT_STATS

BASE=gzip.cpp Locks.cpp root_safety.cpp Scheduler.cpp sigmasks.cpp Work.cpp \
	Worker.cpp Time_nr.cpp util.cpp
filth_SOURCES=$(BASE) HTTP_CacheEntry.cpp HTTP_Client_Work.cpp \
	HTTP_Client_Work_Unix.cpp \
	HTTP_cmdline.cpp HTTP_constants.cpp \
	HTTP_main.cpp HTTP_Origin_Server.cpp \
	HTTP_Server_Work.cpp HTTP_2616_Server_Work.cpp \
	HTTP_parsing.cpp HTTP_Work.cpp
cgi_test_app_SOURCES=$(BASE) CGI_test_app.cpp HTTP_Server_Work.cpp \
	HTTP_constants.cpp HTTP_parsing.cpp HTTP_Work.cpp 
