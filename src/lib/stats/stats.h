#ifndef STATS_H
#define STATS_H

#include <stddef.h>

typedef struct {
    size_t historic_connections;
    size_t current_connections;
    size_t transferred_bytes;
} stats;

void increment_current_connections(); // tambien incrementa historic connections
void decrement_current_connections();
void add_transferred_bytes(size_t bytes_amount);

stats * get_stats();

void cleanup_stats();

#endif