#ifndef STATS_H
#define STATS_H

#include <stddef.h>

typedef struct {
  size_t historic_connections;
  size_t current_connections;
  size_t transferred_bytes;
} stats_t;

void stats_init();

void increment_current_connections(); // tambien incrementa historic connections

void decrement_current_connections();

void add_transferred_bytes(size_t bytes_amount);

stats_t *get_stats();

#endif