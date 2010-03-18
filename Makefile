export BIN_DIR = $(CURDIR)/bin
export DATA_DIR = $(CURDIR)/data
export SRC_DIR = $(CURDIR)/src
export SRC_INC_DIRS = $(SRC_DIR) $(DATA_DIR)
export OBJ_INC_DIRS = $(SRC_DIR)

LOG = _LOG_FATAL

export CPPFLAGS = $(SRC_INC_DIRS:%=-I%) -D $(LOG)
export CXXFLAGS = -std=c++0x
export LDFLAGS = $(OBJ_INC_DIRS:%=-L%)

.PHONY: http
http:
	cd $(SRC_DIR) && $(MAKE) http
.PHONY: standalone-cache
standalone-cache:
	cd $(SRC_DIR) && $(MAKE) standalone-cache
.PHONY: depend
depend:
	cd $(SRC_DIR) && $(MAKE) depend
.PHONY: clean
clean:
	rm -f *~ $(BIN_DIR)/*
	cd $(SRC_DIR) && $(MAKE) clean

