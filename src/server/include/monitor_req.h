#ifndef _MONITOR_REQ_H_
#define _MONITOR_REQ_H_

#include "../../lib/selector/selector.h"
#include "stats.h"
#include "users.h"

#define REQ_AMOUNT 4

/* Possible requests:
**  - ADD USER (parameters: <username> <password> <role>)
**  - REMOVE USER (parameters: <username>)
**  - CHANGE USER PASSWORD (parameters: <username> <password>)
**  - STATISTICS
*/

typedef enum {
  ADD_USER = 0x00,
  REMOVE_USER = 0x01,
  CHANGE_PASSWD = 0x02,
  STATISTICS = 0x03
} monitor_request_t;

#define MAX_ARGUMENT_LENGTH 255

typedef enum {
  MONITOR_REQ_STATE_TYPE = 0,        // reading first byte that determins the type of the request
  MONITOR_REQ_STATE_ARGUMENT_LENGTH, // reading next argument length
  MONITOR_REQ_STATE_ARGUMENT,        // reading argument
  MONITOR_REQ_STATE_DONE,            // done
  MONITOR_REQ_STATE_ERROR            // error during parsing
} monitor_req_state_t;

typedef enum {
  MONITOR_REQ_EVENT_TYPE = 0,
  MONITOR_REQ_EVENT_ARGUMENT_LENGTH,
  MONITOR_REQ_EVENT_ARGUMENT,
  MONITOR_REQ_EVENT_DONE,
  MONITOR_REQ_EVENT_ERROR
} monitor_req_event_t;

typedef enum {
  MONITOR_REQ_STATUS_OK = 0,
  MONITOR_REQ_STATUS_ERR
} monitor_req_status_t;

typedef struct {
  uint8_t type;
  uint8_t arguments_read;
  uint8_t current_argument_length;
  uint8_t current_argument_read;
  monitor_req_status_t req_status;
  char uname[MAX_ARGUMENT_LENGTH + 1];
  char passwd[MAX_ARGUMENT_LENGTH + 1];
  access_level_t access_level;
  stats_t stats;

  struct parser *p;
} monitor_req_parser_t;

void monitor_req_init(const unsigned state, struct selector_key *key);

unsigned monitor_req_read(struct selector_key *key);

unsigned monitor_req_write(struct selector_key *key);

void monitor_req_finalize(const unsigned state, struct selector_key *key);

#endif