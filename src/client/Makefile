include ../../Makefile.inc

SRCS = $(wildcard *.c) $(wildcard ../lib/*/*.c)
OBJECTS = $(SRCS:.c=.o)
EXEC = $(BIN_PATH)/client

all: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEC) $^

clean:
	rm -f $(EXEC) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean all