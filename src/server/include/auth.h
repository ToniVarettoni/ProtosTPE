#ifndef _AUTH_H_
#define _AUTH_H_

#include "../../lib/parser/parser.h"
#include "../../lib/selector/selector.h"
#include "users.h"

#define MAX_BUFFER_SIZE 255
#define MAX_BUFFER 4096

typedef enum {
  AUTH_STATE_VER = 0, // waiting for VER
  AUTH_STATE_ULEN,    // waiting for ULEN
  AUTH_STATE_UNAME,   // waiting for UNAME
  AUTH_STATE_PLEN,    // waiting for PLEN
  AUTH_STATE_PASSWD,  // waiting for PASSWD
  AUTH_STATE_FIN,
  AUTH_STATE_ERROR
} auth_state_t;

typedef enum { AUTH_OK = 0, AUTH_ERROR } auth_status_t;

typedef enum {
  AUTH_EVENT_VER = 0,
  AUTH_EVENT_ULEN,
  AUTH_EVENT_UNAME,
  AUTH_EVENT_PLEN,
  AUTH_EVENT_PASSWD,
  AUTH_EVENT_DONE
} auth_event_t;

typedef struct {
  uint8_t ver;

  uint8_t ulen;
  uint8_t uname[MAX_BUFFER_SIZE];
  uint8_t uname_read;

  uint8_t plen;
  uint8_t passwd[MAX_BUFFER_SIZE];
  uint8_t passwd_read;
  
  uint8_t auth_status;

  struct parser *p;
} auth_parser_t;

void auth_read_init(const unsigned state, struct selector_key *key);

unsigned auth_read(struct selector_key *key);

unsigned auth_write(struct selector_key *key);

#endif