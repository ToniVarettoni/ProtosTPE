#include "include/client_utils.h"
#include "../lib/selector/selector.h"
#include "../lib/stats/stats.h"
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
    size_t buffer_size = strlen(buffer);
    send(key->fd, buffer, buffer_size, 0);
    add_transferred_bytes(buffer_size);
  }
}

void handle_write(struct selector_key *key) {}

void handle_close(struct selector_key *key) {
  decrement_current_connections();
  return;
}

static const fd_handler CLIENT_HANDLER = {.handle_read = handle_read_client,
                                          .handle_close = handle_close};

const fd_handler *get_client_handler() { return &CLIENT_HANDLER; }

const fd_interest CLIENT_INTERESTS = OP_READ;

const fd_interest get_client_interests() { return CLIENT_INTERESTS; }
