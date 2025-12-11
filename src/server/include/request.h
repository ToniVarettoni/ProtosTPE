#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "../../lib/parser/parser.h"
#include "../../lib/selector/selector.h"



#define MAX_ADDR_SIZE 255
#define PORT_SIZE 2
#define IPV4_LEN 4
#define IPV6_LEN 16

void request_read_init(const unsigned state, struct selector_key *key);
unsigned request_read(struct selector_key *key);
unsigned setup_lookup(struct selector_key *key, char *addrname, uint8_t addrlen, uint16_t port);
unsigned dns_lookup(struct selector_key *key);
unsigned try_connect(struct selector_key *key);
unsigned request_write(struct selector_key *key);

typedef struct {
  struct parser *p;
  uint8_t atyp;
  uint8_t cmd;
  uint8_t addr_len;
  uint8_t addr_read;
  uint8_t port_read;
  uint8_t addr[MAX_ADDR_SIZE];
  struct in_addr ipv4;
  struct in6_addr ipv6;
  uint16_t port;
  uint8_t reply_status;
  union {
    struct sockaddr_in sockAddress; 
    struct sockaddr_in6 sockAddress6; 
  };
} request_parser_t;

typedef enum {
  REQUEST_OK = 0,
  REQUEST_CMD_ERROR,
  REQUEST_ATYP_ERROR,
  REQUEST_ADDRESS_ERROR,
  REQUEST_UNKNOWN_ERROR
} request_status_t;

typedef enum {
  ATTYP_IPV4 = 0x01,
  ATTYP_IPV6 = 0x04,
  ATTYP_DOMAINNAME = 0x03
} request_attyp_t;

typedef enum {
  CMD_CONNECT = 0x01,
  CMD_BIND = 0x02,
  CMD_UDP_ASSOCIATE = 0x03
} request_cmd_t;

typedef enum {
  REQUEST_STATE_VER = 0, // waiting for VER field
  REQUEST_STATE_CMD,     // waiting for CMD field
  REQUEST_STATE_RSV,     // waiting for RSV field
  REQUEST_STATE_ATYP,    // waiting for ATYP field
  REQUEST_STATE_DSTADDR_IPV4, // waiting for dst addr field
  REQUEST_STATE_DSTADDR_IPV6, // waiting for dst addr field
  REQUEST_STATE_DSTADDR_DOMAINNAME, // waiting for dst addr field
  REQUEST_STATE_DSTPORT, // waiting for dst port field
  REQUEST_STATE_FIN,     // finished parsing request
  REQUEST_STATE_ERROR    // error while parsing hello
} request_state_t;

#endif
