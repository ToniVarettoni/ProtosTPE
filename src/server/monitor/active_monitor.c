#include "active_monitor.h"
#include "../include/stats.h"

void handle_read_monitor(struct selector_key *key) {
  stm_handler_read(&((monitor_t *)ATTACHMENT(key))->stm, key);
}

void handle_write_monitor(struct selector_key *key) {
  stm_handler_write(&((monitor_t *)ATTACHMENT(key))->stm, key);
}

void handle_close_monitor(struct selector_key *key) {
  stm_handler_close(&((monitor_t *)ATTACHMENT(key))->stm, key);
  decrement_current_connections();
}

unsigned ignore_read_monitor(struct selector_key *key) {
  // ACA HACES LO TUYO TONARDOOOO
  monitor_t *monitor = ATTACHMENT(key);
  return stm_state(&monitor->stm);
}

unsigned ignore_write_monitor(struct selector_key *key) {
  monitor_t *monitor = ATTACHMENT(key);
  return stm_state(&monitor->stm);
}

static const fd_handler ACTIVE_MONITOR_HANDLER = {
    .handle_read = handle_read_monitor,
    .handle_write = handle_write_monitor,
    .handle_close = handle_close_monitor};

const fd_handler *get_active_monitor_handler() {
  return &ACTIVE_MONITOR_HANDLER;
}

const fd_interest INITIAL_ACTIVE_MONITOR_INTERESTS = OP_READ;

const fd_interest get_active_monitor_interests() {
  return INITIAL_ACTIVE_MONITOR_INTERESTS;
}
