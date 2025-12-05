include ../../Makefile.inc

SRCS = $(wildcard *.c) $(wildcard selector/*.c) $(wildcard logger/*.c) $(wildcard forwarding/*.c) $(wildcard closing/*.c) $(wildcard setup/*.c) $(wildcard request/*.c) $(wildcard ../lib/stm/*.c) $(wildcard ../lib/buffer/*.c)
OBJECTS = $(SRCS:.c=.o)
EXEC = $(BIN_PATH)/server
HDRS = ./include

all: $(OBJECTS)
	@echo $(SRCS)
	$(CC) $(CFLAGS) -o $(EXEC) -I $(HDRS) $^

%.o: %.c
	$(CC) $(CFLAGS) -I $(HDRS) -c $< -o $@

clean:
	rm -rf $(EXEC) \
	       $(OBJECTS) \
	       **/*.o \
	       **/*.d \
	       *.o \
	       *.d

.PHONY: clean all