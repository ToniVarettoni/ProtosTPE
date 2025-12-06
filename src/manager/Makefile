include ../../Makefile.inc

SRCS = $(wildcard *.c) $(wildcard ../lib/*/*.c)
OBJECTS = $(SRCS:.c=.o)
EXEC = $(BIN_PATH)/manager
HDRS = -I./../lib

all: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEC) -I $(HDRS) $^

%.o: %.c
	$(CC) $(CFLAGS) -I $(HDRS) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJECTS)

.PHONY: clean all