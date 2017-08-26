
CC=g++
CXXFLAGS=-Wall -Wextra

ts-proc:
	$(CC) -o ts-proc source/main.cpp source/ts_processor.cpp $(CXXFLAGS)

.PHONY: clean

clean:
	rm -rf source/*.o ts-proc