#include "../../lib/buffer/buffer.h"
#include "../../lib/logger/logger.h"
#include "../../lib/selector/selector.h"
#include "../../lib/stm/stm.h"
#include "../include/active_monitor.h"
#include "../include/stats.h"
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

void handle_read_passive_monitor(struct selector_key *key) {
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

  monitor_t *monitor = calloc(1, sizeof(monitor_t));
  if (monitor == NULL) {
    log_to_stdout(
        "Failed to load client with ip: %s, in port: %d: Calloc failed\n",
        inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    close(new_socket);
    return;
  }

  monitor->stm.initial = MONITOR_AUTH;
  monitor->stm.max_state = MONITOR_ERROR;
  monitor->stm.states = monitor_states;
  monitor->active_parser = NO_PARSER;
  // TODO!
  //   buffer_init(&monitor->reading_buffer, MAX_BUFFER,
  //               monitor->reading_buffer_storage);
  //   buffer_init(&monitor->writing_buffer, MAX_BUFFER,
  //               monitor->writing_buffer_storage);
  stm_init(&monitor->stm);

  // add new socket to array of sockets
  selector_status status =
      selector_register(key->s, new_socket,
                        get_active_monitor_handler(),   // handler
                        get_active_monitor_interests(), // interest
                        monitor                         // data
      );

  if (status != SELECTOR_SUCCESS) {
    log_to_stdout("Failed to load client with ip: %s, in port: %d: Selector "
                  "register failed\n",
                  inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    close(new_socket);
    free(monitor);
    return;
  }

  // inform user of socket number - used in send and receive commands
  // printf("New connection , socket fd is %d , ip is : %s , port : %d \n",
  //        new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
  log_to_stdout("New connection , socket fd is %d , ip is : %s , port : %d \n",
                new_socket, inet_ntoa(address.sin_addr),
                ntohs(address.sin_port));
  log_to_stdout("Client connected to monitor!\n");

  // send new connection greeting message
  // int left = strlen(message);
  // left = send_all(new_socket, message, left);
  // if (left != strlen(message)) {
  //   signal();
  // }
}

static const fd_handler PASSIVE_MONITOR_HANDLER = {
    .handle_read = handle_read_passive_monitor};

const fd_handler *get_passive_monitor_handler() {
  return &PASSIVE_MONITOR_HANDLER;
}

const fd_interest INITIAL_PASSIVE_MONITOR_INTERESTS = OP_READ;

const fd_interest get_passive_monitor_interests() {
  return INITIAL_PASSIVE_MONITOR_INTERESTS;
}
