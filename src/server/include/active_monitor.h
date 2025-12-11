#ifndef _MONITOR_H_
#define _MONITOR_H_

#include "../../lib/logger/logger.h"
#include "../../lib/parser/parser.h"
#include "../../lib/selector/selector.h"
#include "../../lib/stm/stm.h"
#include "client_utils.h"
#include "users.h"

#include "monitor_auth.h"
#include "monitor_req.h"

typedef enum {
  MONITOR_AUTH = 0, // waiting for user&password for authentication
  MONITOR_REQ,      // waiting for req from client
  MONITOR_DONE,     // done
  MONITOR_ERROR     // error during monitoring
} monitor_state_t;

typedef enum {
  MONITOR_USER_NOT_FOUND = 1,
  MONITOR_INVALID_USERNAME,
  MONITOR_WRONG_PASSWORD,
  MONITOR_LACKS_PRIVILEGE,
  MONITOR_IO_ERROR,
  MONITOR_INCORRECT_ARGUMENTS,
  MONITOR_REQ_HANDLER_ERROR,
  MONITOR_UNKNOWN_ERROR
} monitor_error_t;

typedef enum {
  NO_PARSER_MONITOR = 0,
  AUTH_PARSER_MONITOR,
  REQ_PARSER
} monitor_parser_state_t;

typedef struct {
  struct state_machine stm;
  access_level_t user_access_level;
  uint8_t error;
  uint8_t active_parser;
  union {
    monitor_auth_parser_t auth_parser;
    monitor_req_parser_t req_parser;
  } parser;
} monitor_t;

unsigned ignore_read_monitor(struct selector_key *key);

unsigned ignore_write_monitor(struct selector_key *key);

static const struct state_definition monitor_states[] = {
    {.state = MONITOR_AUTH,
     .on_arrival = monitor_auth_init,
     .on_read_ready = monitor_auth_read,
     .on_departure = monitor_auth_finalize},
    {.state = MONITOR_REQ,
     .on_arrival = monitor_req_init,
     .on_read_ready = monitor_req_read,
     .on_write_ready = monitor_req_write,
     .on_departure = monitor_req_finalize},
    {.state = MONITOR_DONE},
    {.state = MONITOR_ERROR,
     .on_read_ready = ignore_read_monitor,
     .on_write_ready = ignore_write_monitor}};

void handle_read_monitor(struct selector_key *key);

void handle_write_monitor(struct selector_key *key);

void handle_close_monitor(struct selector_key *key);

const fd_handler *get_active_monitor_handler();

const fd_interest get_active_monitor_interests();

#endif
