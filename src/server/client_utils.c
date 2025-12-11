#define _GNU_SOURCE
#include <netdb.h>
#include "include/client_utils.h"
#include "../lib/selector/selector.h"
#include "include/stats.h"
#include <netdb.h>
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

void destroy_active_parser(client_t *client) {
  if (client == NULL) {
    return;
  }

  switch (client->active_parser) {
  case HELLO_PARSER:
    if (client->parser.hello_parser.p != NULL) {
      parser_destroy(client->parser.hello_parser.p);
      client->parser.hello_parser.p = NULL;
    }
    break;
  case AUTH_PARSER:
    if (client->parser.auth_parser.p != NULL) {
      parser_destroy(client->parser.auth_parser.p);
      client->parser.auth_parser.p = NULL;
    }
    break;
  case REQUEST_PARSER:
    if (client->parser.request_parser.p != NULL) {
      parser_destroy(client->parser.request_parser.p);
      client->parser.request_parser.p = NULL;
    }
    break;
  default:
    break;
  }
  client->active_parser = NO_PARSER;
}

void free_destination(client_t *client) {
  if (client == NULL || client->dest_addr_base == NULL) {
    return;
  }

  if (client->dest_addr_from_gai) {
    freeaddrinfo(client->dest_addr_base);
  } else {
    free(client->dest_addr_base);
  }

  client->dest_addr = NULL;
  client->dest_addr_base = NULL;
  client->dest_addr_from_gai = false;
}

void free_dns_request(client_t *client) {
  if (client == NULL || client->dns_req == NULL) {
    return;
  }
  if (client->dns_req->ar_request != NULL) {
    free((void *)client->dns_req->ar_request);
  }
  if (client->dns_req->ar_service != NULL) {
    free((void *)client->dns_req->ar_service);
  }
  if (client->dns_req->ar_name != NULL) {
    free((void *)client->dns_req->ar_name);
  }
  free(client->dns_req);
  client->dns_req = NULL;
}

void handle_write_client(struct selector_key *key) {
  stm_handler_write(&((client_t *)ATTACHMENT(key))->stm, key);
}

void handle_close_client(struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  stm_handler_close(&client->stm, key);

  if (key->fd == client->client_fd) {
    decrement_current_connections();
  }
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
