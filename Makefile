
CC = g++
CXXFLAGS = -Wall -Wextra -Werror

VERSTR := $(shell cat VERSION)

ts-proc:
	echo $(VERSTR)
	$(CC) -o ts-proc source/main.cpp source/ts_processor.cpp $(CXXFLAGS) -DVERSION='"$(VERSTR)"'

.PHONY: clean

clean:
	rm -rf source/*.o ts-proc