#include "../lib/buffer/buffer.h"
#include "../lib/logger/logger.h"
#include "../lib/selector/selector.h"
#include "../lib/stm/stm.h"
#include "include/client_utils.h"
#include "include/stats.h"
#include "io_utils.h"
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
  struct sockaddr_storage address;
  socklen_t addr_len = sizeof(address);
  int new_socket;

  do {
    addr_len = sizeof(address);
    new_socket =
        accept(key->fd, (struct sockaddr *)&address, (socklen_t *)&addr_len);
    if (new_socket < 0) {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;
      perror("accept");
      exit(EXIT_FAILURE);
    }
  } while (new_socket < 0);

  int flags = fcntl(new_socket, F_GETFL, 0);
  fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);

  client_t *client = calloc(1, sizeof(client_t));
  if (client == NULL) {
    log_to_stdout(
        "Failed to load client with ip: %s, in port: %d: Calloc failed\n",
        "unknown", 0);
    close(new_socket);
    return;
  }

  client->client_fd = new_socket;
  client->stm.initial = HELLO_READ;
  client->stm.max_state = ERROR;
  client->stm.states = client_states;
  client->destination_fd = -1;
  client->active_parser = NO_PARSER;
  buffer_init(&client->client_buffer, MAX_BUFFER,
              client->client_buffer_storage);
  buffer_init(&client->destiny_buffer, MAX_BUFFER,
              client->destiny_buffer_storage);

  stm_init(&client->stm);

  // add new socket to array of sockets
  selector_status status = selector_register(key->s, new_socket,
                                             get_client_handler(),   // handler
                                             get_client_interests(), // interest
                                             client                  // data
  );

  if (status != SELECTOR_SUCCESS) {
    char host[INET6_ADDRSTRLEN] = {0};
    uint16_t port = 0;
    if (address.ss_family == AF_INET) {
      struct sockaddr_in *addr4 = (struct sockaddr_in *)&address;
      inet_ntop(AF_INET, &addr4->sin_addr, host, sizeof(host));
      port = ntohs(addr4->sin_port);
    } else if (address.ss_family == AF_INET6) {
      struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&address;
      inet_ntop(AF_INET6, &addr6->sin6_addr, host, sizeof(host));
      port = ntohs(addr6->sin6_port);
    }
    log_to_stdout("Failed to load client with ip: %s, in port: %d: Selector "
                  "register failed\n",
                  host, port);
    close(new_socket);
    free(client);
    return;
  }
  increment_current_connections();

  // inform user of socket number - used in send and receive commands
  // printf("New connection , socket fd is %d , ip is : %s , port : %d \n",
  //        new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
  char host[INET6_ADDRSTRLEN] = {0};
  uint16_t port = 0;
  if (address.ss_family == AF_INET) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *)&address;
    inet_ntop(AF_INET, &addr4->sin_addr, host, sizeof(host));
    port = ntohs(addr4->sin_port);
  } else if (address.ss_family == AF_INET6) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&address;
    inet_ntop(AF_INET6, &addr6->sin6_addr, host, sizeof(host));
    port = ntohs(addr6->sin6_port);
  }
  log_to_stdout("New connection , socket fd is %d , ip is : %s , port : %d \n",
                new_socket, host, port);

  // send new connection greeting message
  // int left = strlen(message);
  // left = send_all(new_socket, message, left);
  // if (left != strlen(message)) {
  //   signal();
  // }
}

static const fd_handler MASTER_HANDLER = {.handle_read = handle_read_master};

const fd_handler *get_master_handler() { return &MASTER_HANDLER; }
