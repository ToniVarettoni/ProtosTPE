include ../../Makefile.inc

SRCS = $(wildcard *.c)
OBJECTS = $(SRCS:.c=.o)
EXEC = $(BIN_PATH)/client

all: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEC) $^

clean:
	rm -f $(EXEC) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean all