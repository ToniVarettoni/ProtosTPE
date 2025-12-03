#include "../client_utils.h"
#include "../selector/selector.h"
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

// TODO: handle errors!! GAYmer

void handle_read_master(struct selector_key *key) {
  int new_socket, addr_len;
  struct sockaddr_in address;
  char *message = "ECHO Daemon v1.0 \r\n";

  if ((new_socket = accept(key->fd, (struct sockaddr *)&address,
                           (socklen_t *)&addr_len)) < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  // inform user of socket number - used in send and receive commands
  printf("New connection , socket fd is %d , ip is : %s , port : %d \n",
         new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

  // send new connection greeting message
  if (send(new_socket, message, strlen(message), 0) != strlen(message)) {
    perror("send");
  }

  puts("Welcome message sent successfully");

  // add new socket to array of sockets
  selector_register(key->s, new_socket,
                    get_client_handler(), // handler
                    CLIENT_INTERESTS,     // interest
                    NULL);                // data
}
