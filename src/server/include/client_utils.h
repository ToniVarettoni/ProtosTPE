#ifndef _CLIENT_UTILS_
#define _CLIENT_UTILS_

#include "../../lib/buffer/buffer.h"
#include "../../lib/selector/selector.h"
#include "../../lib/stm/stm.h"
#include "auth.h"
#include "closing.h"
#include "forwarding.h"
#include "hello.h"
#include "request.h"

#define MAX_BUFFER 4096

typedef struct {
  struct state_machine stm;
  buffer reading_buffer;
  buffer writing_buffer;
  uint8_t reading_buffer_storage[MAX_BUFFER];
  uint8_t writing_buffer_storage[MAX_BUFFER];
  int destination_fd;
} client_t;

enum states {
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
};

static const struct state_definition client_states[] = {
    {.state = HELLO_READ, .on_read_ready = hello_read},
    {.state = HELLO_WRITE, .on_write_ready = hello_write},
    {.state = AUTH_READ, .on_read_ready = auth_read},
    {.state = AUTH_WRITE, .on_write_ready = auth_write},
    {.state = REQUEST_READ, .on_read_ready = request_read},
    {
        .state = DNS_LOOKUP,
        .on_arrival = dns_lookup,
    },
    {.state = DEST_CONNECT, .on_arrival = try_connect},
    {.state = REQUEST_WRITE, .on_write_ready = request_write},
    {.state = FORWARDING,
     .on_write_ready = forward_write,
     .on_read_ready = forward_read,
     .on_arrival = forward_setup,
     .on_departure = forward_close},
    {.state = DONE, .on_arrival = end_connection},
    {.state = ERROR, .on_arrival = error_handler}};

const fd_interest get_client_interests();

const fd_handler *get_client_handler();

#endif
