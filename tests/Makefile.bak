TEST_PROGNAME ?= tests
TEST_SRCS = test_main.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
# There may be one or more object files in the source directory that
# already define main(). This option just pays attention to the first
# definition, which we ensure below is *our* main().
LDFLAGS += -Xlinker --allow-multiple-definition

tests: $(TEST_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -lpthread $(TEST_OBJS) \
	$(SRC_DIR)/*.o -o $(TEST_PROGNAME)

.PHONY: depend
depend:
	makedepend -Y $(SRC_INC_DIRS:%=-I%) -Y $(TEST_SRCS) 2> /dev/null
.PHONY: clean
clean:
	rm -f *.o *~



# DO NOT DELETE

test_main.o: FileCacheTest.hpp /home/brendan/http/src/FileCache.hpp
test_main.o: /home/brendan/http/src/LockedQueue.hpp
test_main.o: /home/brendan/http/src/Locks.hpp
test_main.o: /home/brendan/http/src/FindWork.hpp
test_main.o: /home/brendan/http/src/Work.hpp
test_main.o: /home/brendan/http/src/Callback.hpp
test_main.o: /home/brendan/http/src/Workmap.hpp
test_main.o: /home/brendan/http/src/Scheduler.hpp
test_main.o: /home/brendan/http/src/Thread.hpp
test_main.o: /home/brendan/http/src/logging.h
test_main.o: /home/brendan/http/src/sigmasks.hpp
