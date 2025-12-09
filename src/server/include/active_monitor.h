#ifndef _MONITOR_H_
#define _MONITOR_H_

#include "../../lib/stm/stm.h"
#include "monitor_auth.h"
#include "monitor_req.h"
#include "monitor_res.h"
#include "users.h"

typedef enum {
  MONITOR_AUTH = 0,  // waiting for user&password for authentication
  MONITOR_REQ_READ,  // waiting for req from client
  MONITOR_RES_WRITE, // writing response to client
  MONITOR_DONE,      // done
  MONITOR_ERROR      // error during monitoring
} monitor_state_t;

typedef struct {
  struct state_machine stm;
  access_level_t user_access_level;
  union {
    monitor_auth_parser_t auth_parser;
  } parser;
} monitor_t;

static const struct state_definition monitor_states[] = {
    {.state = MONITOR_AUTH,
     .on_arrival = monitor_init,
     .on_read_ready = monitor_auth_read},
    {.state = MONITOR_REQ_READ,
     .on_arrival = monitor_read_init,
     .on_read_ready = monitor_req_read,
     .on_departure = monitor_req_read_finalize},
    {.state = MONITOR_RES_WRITE,
     .on_arrival = monitor_res_write_init,
     .on_read_ready = monitor_res_write,
     .on_departure = monitor_res_write_finalize},
    {.state = MONITOR_DONE},
    {.state = MONITOR_ERROR}};

void handle_read_monitor(struct selector_key *key);

void handle_write_monitor(struct selector_key *key);

void handle_close_monitor(struct selector_key *key);

unsigned ignore_read_monitor(struct selector_key *key);

unsigned ignore_write_monitor(struct selector_key *key);

const fd_handler *get_active_monitor_handler();

const fd_interest get_active_monitor_interests();

#endif