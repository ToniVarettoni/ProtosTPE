include ../../Makefile.inc

SRCS = $(wildcard *.c) $(wildcard selector/*.c)
OBJECTS = $(SRCS:.c=.o)
EXEC = $(BIN_PATH)/server
HDRS = ./include

all: $(OBJECTS)
	@echo $(SRCS)
	$(CC) $(CFLAGS) -o $(EXEC) -I $(HDRS) $^

%.o: %.c
	$(CC) $(CFLAGS) -I $(HDRS) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJECTS)

.PHONY: clean all