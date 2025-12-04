CC =gcc
CFLAGS =-std=c11 -Wall -pthread

BIN_PATH = ../../bin

GLOBAL_HDRS = 

ifneq ($(DEBUG), 0)
    CFLAGS += -g -DDEBUG
else
    CFLAGS += -O2
endif