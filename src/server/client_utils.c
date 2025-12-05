#include "include/client_utils.h"
#include "include/selector.h"
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
#include "stm.h"

static enum states {
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
  {
    .state = HELLO_READ,
    .on_read_ready = read_hello
  },
  {
    .state = HELLO_WRITE,
    .on_write_ready = write_hello
  },
  {
    .state = AUTH_READ,
    .on_read_ready = read_auth
  },
  {
    .state = AUTH_WRITE,
    .on_write_ready = write_auth
  },
  {
    .state = REQUEST_READ,
    .on_read_ready = read_request
  },
  {
    .state = DNS_LOOKUP,
    .on_arrival = dns_lookup,
    .on_block_ready = NOSE
  },
  {
    .state = DEST_CONNECT,
    .on_arrival = try_connect
  },
  {
    .state = REQUEST_WRITE,
    .on_write_ready = write_request
  },
  {
    .state = FORWARDING,
    .on_write_ready = write_forward,
    .on_read_ready = read_forward,
    .on_arrival = setup_forward,
    .on_departure = close_forward
  },
  {
    .state = DONE,
    .on_arrival = end_connection
  },
  {
    .state = ERROR,
    .on_arrival = error_handler
  }
};

void handle_read_client(struct selector_key *key) {
  // Check if it was for closing , and also read the incoming message
  int addr_len, valread;
  struct sockaddr_in address;

  char buffer[1025]; // data buffer of 1K
  selector_status error;

  if ((valread = read(key->fd, buffer, 1024)) == 0) {
    // Somebody disconnected , get his details and print
    getpeername(key->fd, (struct sockaddr *)&address, (socklen_t *)&addr_len);
    printf("Host disconnected , ip %s , port %d \n",
           inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    // Close the socket and mark as 0 in list for reuse
    close(key->fd);
    if ((error = selector_unregister_fd(key->s, key->fd)) != SELECTOR_SUCCESS) {
      printf("%s\n", selector_error(error));
    }
  }

  // Echo back the message that came in
  else {
    // set the string terminating NULL byte on the end of the data read
    buffer[valread] = '\0';
    send(key->fd, buffer, strlen(buffer), 0);
  }
}

void handle_write(struct selector_key *key) {}

void handle_close(struct selector_key *key) { return; }

static const fd_handler CLIENT_HANDLER = {.handle_read = handle_read_client,
                                          .handle_close = handle_close};

const fd_handler *get_client_handler() { return &CLIENT_HANDLER; }

const fd_interest CLIENT_INTERESTS = OP_READ;

const fd_interest get_client_interests() { return CLIENT_INTERESTS; }
