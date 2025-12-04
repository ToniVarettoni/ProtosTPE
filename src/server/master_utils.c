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

void handle_read_master(struct selector_key *key) {
  selector_status error;
  struct sockaddr_in address;
  int new_socket, addr_len = 0;
  char *message = "ECHO Daemon v1.0 \r\n";

  if ((new_socket = accept(key->fd, (struct sockaddr *)&address,
                           (socklen_t *)&addr_len)) < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  int flags = fcntl(new_socket, F_GETFL, 0);
  fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);

  // inform user of socket number - used in send and receive commands
  printf("New connection , socket fd is %d , ip is : %s , port : %d \n",
         new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

  // send new connection greeting message
  if (send(new_socket, message, strlen(message), 0) != strlen(message)) {
    perror("send");
  }

  puts("Welcome message sent successfully");

  // add new socket to array of sockets
  if ((error = selector_register(key->s, new_socket,
                                 get_client_handler(),   // handler
                                 get_client_interests(), // interest
                                 NULL)) != SELECTOR_SUCCESS) {
    printf("%s\n", selector_error(error));
  } // data
}

static const fd_handler MASTER_HANDLER = {.handle_read = handle_read_master};

const fd_handler *get_master_handler() { return &MASTER_HANDLER; }
