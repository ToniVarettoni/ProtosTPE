#include "stats.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static stats_t current_stats;

void stats_init() { memset(&current_stats, 0, sizeof(stats_t)); }

void increment_current_connections() {
  current_stats.historic_connections++;
  current_stats.current_connections++;
}

void decrement_current_connections() { current_stats.current_connections--; }

void add_transferred_bytes(size_t bytes_amount) {
  current_stats.transferred_bytes += bytes_amount;
}

stats_t *get_stats() { return &current_stats; }
