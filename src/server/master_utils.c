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
  struct sockaddr_in address;
  int new_socket, addr_len = 0;

  do {
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
        inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    close(new_socket);
    return;
  }
  increment_current_connections();

  client->client_fd = new_socket;
  client->stm.initial = HELLO_READ;
  client->stm.max_state = ERROR;
  client->stm.states = client_states;
  client->destination_fd = -1;
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
    log_to_stdout("Failed to load client with ip: %s, in port: %d: Selector "
                  "register failed\n",
                  inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    close(new_socket);
    free(client);
    return;
  }

  // inform user of socket number - used in send and receive commands
  // printf("New connection , socket fd is %d , ip is : %s , port : %d \n",
  //        new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
  log_to_stdout("New connection , socket fd is %d , ip is : %s , port : %d \n",
                new_socket, inet_ntoa(address.sin_addr),
                ntohs(address.sin_port));

  // send new connection greeting message
  // int left = strlen(message);
  // left = send_all(new_socket, message, left);
  // if (left != strlen(message)) {
  //   signal();
  // }
}

static const fd_handler MASTER_HANDLER = {.handle_read = handle_read_master};

const fd_handler *get_master_handler() { return &MASTER_HANDLER; }
