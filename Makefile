SRC_DIR = src
DATA_DIR = data
TEST_DIR = tests


export SRC_INC_DIRS = $(CURDIR)/$(SRC_DIR) $(CURDIR)/$(DATA_DIR)
export OBJ_INC_DIRS = $(CURDIR)/$(SRC_DIR)
export CPPFLAGS = $(SRC_INC_DIRS:%=-I%) -D _LOG_FATAL
export CXXFLAGS = -std=c++0x
export LDFLAGS = $(OBJ_INC_DIRS:%=-L%)

.PHONY: http
http:
	cd $(SRC_DIR) && $(MAKE) http
.PHONY: tests
tests:
	cd $(TEST_DIR) && $(MAKE)
.PHONY: depend
depend:
	cd $(SRC_DIR) && $(MAKE) depend
.PHONY: clean
clean:
	rm -f Makefile~
	cd $(SRC_DIR) && $(MAKE) clean
	cd $(TEST_DIR) && $(MAKE) clean