.SUFFIXES:

MAKEFLAGS    += --jobs=$(shell nproc)
CC            = gcc
LDFLAGS       = -lpthread -lm -lstdc++ $(shell pkg-config --libs libpulse libpulse-simple) -lmp3lame
CFLAGS_ALL    = -Isrc -Ijson/include -Wno-unused-result -Wno-format-truncation -Wno-narrowing -std=c++17 $(shell pkg-config --cflags libpulse libpulse-simple)
CFLAGS        = $(CFLAGS_ALL)
CFLAGS_DEBUG  = $(CFLAGS_ALL) -O0 -ggdb -DDEBUG
HEADERS       = $(shell find ./src -type f -name "*.h")
SOURCES       = $(shell find ./src -type f -name "*.c" -o -name "*.cpp")
OBJECTS       = $(shell find ./src -type f -name "*.c" -o -name "*.cpp" -printf '%p.o\n' | sed -e 's/src/build/')
OBJECTS_DEBUG = $(shell find ./src -type f -name "*.c" -o -name "*.cpp" -printf '%p.debug.o\n' | sed -e 's/src/build/')
TARGET        = musicsys

.PHONY: all debug debug-run run clean

all: ./build/main.o
	cp $< ./$(TARGET)
	# sudo setcap CAP_NET_BIND_SERVICE=+eip $(TARGET)

debug: ./build/debug.o
	cp $< ./$(TARGET)
	# sudo setcap CAP_NET_BIND_SERVICE=+eip $(TARGET)

./build/main.o: $(OBJECTS)
	@echo LD $@
	@$(CC) -o $@ $^ $(LDFLAGS)

./build/%.o: ./src/% $(HEADERS) Makefile
	@mkdir -p $(shell dirname $@)
	@echo CC $<
	@$(CC) -c -o $@ $< $(CFLAGS)

./build/debug.o: $(OBJECTS_DEBUG)
	@echo LD $@
	@$(CC) -o $@ $^ $(LDFLAGS)

./build/%.debug.o: ./src/% $(HEADERS) Makefile
	@mkdir -p $(shell dirname $@)
	@echo CC $<
	@$(CC) -c -o $@ $< $(CFLAGS_DEBUG)

debug-run: debug
	./$(TARGET)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -rf ./build/*