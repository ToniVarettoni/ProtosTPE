#ifndef _MONITOR_UTILS_H_
#define _MONITOR_UTILS_H_

#include "../../lib/selector/selector.h"

#define MAX_LENGTH 255

typedef enum {
  MONITOR_AUTH_STATE_ULEN = 0,
  MONITOR_AUTH_STATE_UNAME,
  MONITOR_AUTH_STATE_PLEN,
  MONITOR_AUTH_STATE_PASSWD,
  MONITOR_AUTH_STATE_DONE,
  MONITOR_AUTH_STATE_ERROR
} monitor_auth_state_t;

typedef enum {
  MONITOR_AUTH_EVENT_ULEN = 0,
  MONITOR_AUTH_EVENT_UNAME,
  MONITOR_AUTH_EVENT_PLEN,
  MONITOR_AUTH_EVENT_PASSWD,
  MONITOR_AUTH_EVENT_DONE,
  MONITOR_AUTH_EVENT_ERROR
} monitor_auth_event_t;

typedef struct {
  uint8_t ulen;
  uint8_t plen;
  uint8_t uname_read, passwd_read;

  char uname[MAX_LENGTH];
  char passwd[MAX_LENGTH];

  struct parser *p;
} monitor_auth_parser_t;

typedef enum {
  MONITOR_AUTH_STATUS_OK = 0,
  MONITOR_AUTH_STATUS_ERR
} monitor_auth_status_t;

void monitor_auth_init(const unsigned state, struct selector_key *key);

unsigned monitor_auth_read(struct selector_key *key);

void monitor_auth_finalize(const unsigned state, struct selector_key *key);

#endif