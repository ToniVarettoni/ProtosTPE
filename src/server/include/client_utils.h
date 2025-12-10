#ifndef _CLIENT_UTILS_
#define _CLIENT_UTILS_
#define __USE_XOPEN2K 1
#define __USE_GNU
#define __USE_MISC

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../../lib/buffer/buffer.h"
#include "../../lib/selector/selector.h"
#include "../../lib/stm/stm.h"
#include "auth.h"
#include "closing.h"
#include "forwarding.h"
#include "hello.h"
#include "request.h"

typedef struct addrinfo addrinfo;

#define MAX_BUFFER 4096

typedef struct {
  struct state_machine stm;
  uint8_t client_fd;
  uint8_t active_parser;
  union {
    hello_parser_t hello_parser;
    auth_parser_t auth_parser;
    request_parser_t request_parser;
  } parser;
  buffer client_buffer;
  buffer destiny_buffer;
  uint8_t client_buffer_storage[MAX_BUFFER];
  uint8_t destiny_buffer_storage[MAX_BUFFER];
  int destination_fd;
  struct addrinfo *dest_addr;
  struct gaicb * dns_req;
  uint8_t err;
} client_t;

typedef enum {
  NO_PARSER = 0,
  HELLO_PARSER,
  AUTH_PARSER,
  REQUEST_PARSER
} parser_state_t;

typedef enum {
  HELLO_READ = 0,
  HELLO_WRITE,
  AUTH_READ,
  AUTH_WRITE,
  REQUEST_READ,
  DNS_LOOKUP,
  DEST_CONNECT,
  REQUEST_WRITE,
  FORWARDING,
  DONE,
  ERROR
} client_state_t;

static const struct state_definition client_states[] = {
    {.state = HELLO_READ,
     .on_arrival = hello_read_init,
     .on_read_ready = hello_read},
    {.state = HELLO_WRITE,
     .on_write_ready = hello_write},
    {.state = AUTH_READ,
     .on_arrival = auth_read_init,
     .on_read_ready = auth_read},
    {.state = AUTH_WRITE,
     .on_write_ready = auth_write},
    {.state = REQUEST_READ,
     .on_arrival = request_read_init,
     .on_read_ready = request_read},
    {.state = DNS_LOOKUP,
     .on_write_ready = dns_lookup},
    {.state = DEST_CONNECT,
     .on_write_ready = try_connect},
    {.state = REQUEST_WRITE,
     .on_write_ready = request_write},
    {.state = FORWARDING,
     .on_write_ready = forward_write,
     .on_read_ready = forward_read},
    {.state = DONE,
     .on_arrival = end_connection},
    {.state = ERROR,
     .on_arrival = error_handler}};

const fd_interest get_client_interests();

const fd_handler *get_client_handler();

void close_connection(struct selector_key *key);

#endif
