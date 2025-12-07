#ifndef _HELLO_H_
#define _HELLO_H_

#include "../../lib/parser/parser.h"
#include "../../lib/selector/selector.h"

#define HELLO_MAX_METHODS 255
#define MAX_BUFFER 4096

typedef enum {
  HELLO_STATE_VER = 0,  // waiting for VER field
  HELLO_STATE_NMETHODS, // waiting for NMETHODS field
  HELLO_STATE_METHODS,  // waiting for METHODS field
  HELLO_STATE_FIN,      // finished parsing hello
  HELLO_STATE_ERROR     // error while parsing hello
} hello_state_t;

typedef enum {
  HELLO_AUTH_NO_AUTH = 0x00,
  HELLO_AUTH_USER_PASS = 0x02,
  HELLO_AUTH_NO_METHOD_ACCEPTED = 0xFF
} hello_auth_method_t;

typedef enum {
  HELLO_OK = 0,
  HELLO_VER_ERROR,
  HELLO_NMETHODS_ERROR,
  HELLO_METHOD_NOT_ACCEPTED_ERROR,
  HELLO_UNKNOWN_ERROR
} hello_status_t;

typedef enum {
  HELLO_EVENT_VER = 0,
  HELLO_EVENT_NMETHODS,
  HELLO_EVENT_METHOD,
  HELLO_EVENT_DONE,
  HELLO_EVENT_UNEXPECTED
} hello_event_t;

struct hello_parser {
  uint8_t ver;
  uint8_t nmethods;
  uint8_t methods[HELLO_MAX_METHODS];
  uint8_t methods_read;

  uint8_t method_selected;

  struct parser *p;
};

hello_status_t hello_init(struct hello_parser *hp);

hello_status_t hello_read(struct selector_key *key);

hello_status_t hello_write(struct selector_key *key);

#endif