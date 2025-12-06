#include "stats.h"
#include <stddef.h>

static stats current_stats = {
    .historic_connections = 0,
    .current_connections = 0,
    .transferred_bytes = 0,
};

void increment_current_connections() {
    current_stats.historic_connections++;
    current_stats.current_connections++;
}

void decrement_current_connections() {
    current_stats.current_connections--;
}

void add_transferred_bytes(size_t bytes_amount) {
    current_stats.transferred_bytes += amount;
}

stats get_stats() {
    return current_stats;
}

