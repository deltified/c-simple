CC := g++
CFLAGS := -std=c++17 -O2 -Iinclude
SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:.cpp=.o)
BIN := bin/run

.PHONY: all clean
all: $(BIN)

$(BIN): $(SRCS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(BIN) $(OBJS)
