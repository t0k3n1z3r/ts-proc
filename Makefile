
CC=g++
CXXFLAGS=-Wall -Wextra -std=c++11

ts-proc:
	$(CC) -o ts-proc source/main.cpp source/ts_processor.cpp \
	source/log.cpp source/es_writer.cpp $(CXXFLAGS)

.PHONY: clean

clean:
	rm -rf source/*.o ts-proc