export BIN_DIR = $(CURDIR)/bin
export DATA_DIR = $(CURDIR)/data
export OBJ_DIR = $(CURDIR)/obj
export SRC_DIR = $(CURDIR)/src
export TEST_DIR = $(CURDIR)/tests
export SRC_INC_DIRS = $(SRC_DIR) $(DATA_DIR)
export OBJ_INC_DIRS = $(OBJ_DIR)

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
	rm -f *~ $(BIN_DIR)/* $(OBJ_DIR)/* $(SRC_DIR)/*~ $(SRC_DIR)/*\# \
		$(TEST_DIR)/*~ $(TEST_DIR)/*\#
