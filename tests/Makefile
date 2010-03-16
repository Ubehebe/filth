TEST_SRCS = LockedQueueTest.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
# There may be one or more object files in the source directory that
# already define main(). This option just pays attention to the first
# definition, which we ensure below is *our* main().
LDFLAGS += -Xlinker --allow-multiple-definition

all: $(TEST_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -lpthread $(TEST_OBJS) \
	$(OBJ_INC_DIRS:%=%/*.o) -o tests
.PHONY: clean
clean:
	rm -f tests



