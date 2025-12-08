/**
    Handle multiple socket connections with select and fd_set on Linux
*/

#include "../lib/logger/logger.h"
#include "../lib/selector/selector.h"
#include "../lib/stats/stats.h"
#include "include/master_utils.h"
#include "include/users.h"
#include <arpa/inet.h> //close
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <unistd.h> //close

#define TRUE 1
#define FALSE 0
#define PORT 8888

#define MAX_CLIENTS 30

static int running = true;

void signal_handler(int signal) { running = false; }

int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  const struct selector_init sel_init = {.signal = 0};

  selector_init(&sel_init);

  fd_selector fds = selector_new(MAX_CLIENTS);
  selector_status error;

  int opt = TRUE;
  int master_socket;
  struct sockaddr_in address;

  // create a master socket
  if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  int flags = fcntl(master_socket, F_GETFL, 0);
  fcntl(master_socket, F_SETFL, flags | O_NONBLOCK);

  // set master socket to allow multiple connections , this is just a good
  // habit, it will work without this
  if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                 sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // type of socket created
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // bind the socket to localhost port 8888
  if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // try to specify maximum of 3 pending connections for the master socket
  if (listen(master_socket, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  if ((error = selector_register(fds, master_socket, get_master_handler(),
                                 MASTER_INTERESTS, NULL)) != SELECTOR_SUCCESS) {
    log_to_stdout("%s\n", selector_error(error));
  }

  // initialize my logger with my selector
  logger_initialize(fds);

  // initialize users from users.txt
  user_status user_init_status = users_init(NULL);
  if (user_init_status != USERS_OK) {
    log_to_stdout("Warning: Failed to initialize users. Authentication may not work.\n");
  } else {
    log_to_stdout("Users loaded successfully from %s\n", DEFAULT_USERS_FILE_PATH);
  }

  log_to_stdout("Listener on port %d \n", PORT);

  while (running) {
    selector_select(fds);
  }

  log_to_stdout("\nShutting down server...\n");

  users_shutdown();
  logger_destroy();
  close(master_socket);
  selector_destroy(fds);
  selector_close();
  cleanup_stats();

  return 0;
}