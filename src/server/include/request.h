#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "../../lib/selector/selector.h"
#include "../../lib/parser/parser.h"
#define MAX_ADDR_SIZE 255
#define PORT_SIZE 2

void request_read_init(const unsigned state, struct selector_key *key);
unsigned request_read(struct selector_key *key);
void dns_lookup(const unsigned state, struct selector_key *key);
void try_connect(const unsigned state, struct selector_key *key);
unsigned request_write(struct selector_key *key);

struct request_parser {
    struct parser * p;
    uint8_t atyp;
    uint8_t cmd;
    uint8_t addr_len;
    uint8_t dst_port[PORT_SIZE];
    uint8_t dst_addr[MAX_ADDR_SIZE];
};

typedef enum {
  REQUEST_STATE_VER = 0,  // waiting for VER field
  REQUEST_STATE_CMD,      // waiting for CMD field
  REQUEST_STATE_RSV,      // waiting for RSV field
  REQUEST_STATE_ATYP,     // waiting for ATYP field
  REQUEST_STATE_DSTADDR,  // waiting for dst addr field
  REQUEST_STATE_DSTPORT,  // waiting for dst port field
  REQUEST_STATE_FIN,      // finished parsing request
  REQUEST_STATE_ERROR     // error while parsing hello
} request_state_t;

#endif