#include "include/client_utils.h"
#include "../lib/selector/selector.h"
#include "include/stats.h"
#include <arpa/inet.h> //close
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <unistd.h> //close

void handle_read_client(struct selector_key *key) {
  stm_handler_read(&((client_t *)ATTACHMENT(key))->stm, key);
}

void handle_write_client(struct selector_key *key) {
  stm_handler_write(&((client_t *)ATTACHMENT(key))->stm, key);
}

void handle_close_client(struct selector_key *key) {
  stm_handler_close(&((client_t *)ATTACHMENT(key))->stm, key);
  increment_current_connections();
}

unsigned ignore_read(struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  return stm_state(&client->stm);
}

unsigned ignore_write(struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  return stm_state(&client->stm);
}

static const fd_handler CLIENT_HANDLER = {.handle_read = handle_read_client,
                                          .handle_write = handle_write_client,
                                          .handle_close = handle_close_client};

const fd_handler *get_client_handler() { return &CLIENT_HANDLER; }

const fd_interest INITIAL_CLIENT_INTERESTS = OP_READ;

const fd_interest get_client_interests() { return INITIAL_CLIENT_INTERESTS; }

void close_connection(struct selector_key *key) {
  // TODO
}
