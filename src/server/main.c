/**
    Handle multiple socket connections with select and fd_set on Linux
*/

#include "../lib/args/args.h"
#include "../lib/logger/logger.h"
#include "../lib/selector/selector.h"
#include "include/master_utils.h"
#include "include/passive_monitor.h"
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

#define PORT 1080
#define MONITOR_PORT 8080

#define DEFAULT_MAX_CLIENTS 1000
#define LISTEN_BACKLOG 512

#define log_file "logs.txt"

static int running = true;

void signal_handler(int signal) { running = false; }

int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  struct socks5args args;
  parse_args(argc, argv, &args);
  user_status user_init_status = users_init(NULL);

  // creates users added from command line
  for (int i = 0; i < MAX_USERS && args.users[i].name != NULL; i++) {
    user_create(args.users[i].name, args.users[i].pass, USER);
  }

  const struct selector_init sel_init = {
      .signal = 0, .select_timeout = {.tv_sec = 60, .tv_nsec = 0}};

  selector_init(&sel_init);

  fd_selector fds = selector_new(DEFAULT_MAX_CLIENTS);
  selector_status error;

  int opt = TRUE;
  int master_socket;
  struct sockaddr_in6 address6;

  // create a master socket, listening for IPv6 AND IPv4
  if ((master_socket = socket(AF_INET6, SOCK_STREAM, 0)) == 0) {
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

  // allow IPv4-mapped connections on the IPv6 socket
  int v6only = 0;
  if (setsockopt(master_socket, IPPROTO_IPV6, IPV6_V6ONLY, &v6only,
                 sizeof(v6only)) < 0) {
    perror("setsockopt IPV6_V6ONLY");
    exit(EXIT_FAILURE);
  }

  // type of socket created
  memset(&address6, 0, sizeof(address6));
  address6.sin6_family = AF_INET6;
  address6.sin6_port = htons(args.socks_port);
  if (inet_pton(AF_INET6, args.socks_addr, &address6.sin6_addr) <= 0) {
    // default to any if provided addr is not valid IPv6 (e.g., IPv4 string)
    address6.sin6_addr = in6addr_any;
  }

  // bind the socket to the port
  if (bind(master_socket, (struct sockaddr *)&address6, sizeof(address6)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // allow a large backlog to tolerate connection spikes
  if (listen(master_socket, LISTEN_BACKLOG) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  if ((error = selector_register(fds, master_socket, get_master_handler(),
                                 MASTER_INTERESTS, NULL)) != SELECTOR_SUCCESS) {
    log_to_stdout("%s\n", selector_error(error));
  }

  log_to_stdout("Listener on port %d \n", args.socks_port);

  int monitor_socket;

  // create a master socket, listening for IPv6 AND IPv4
  if ((monitor_socket = socket(AF_INET6, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  flags = fcntl(monitor_socket, F_GETFL, 0);
  fcntl(monitor_socket, F_SETFL, flags | O_NONBLOCK);

  // set master socket to allow multiple connections , this is just a good
  // habit, it will work without this
  if (setsockopt(monitor_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                 sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // allow IPv4-mapped connections on the IPv6 socket
  v6only = 0;
  if (setsockopt(monitor_socket, IPPROTO_IPV6, IPV6_V6ONLY, &v6only,
                 sizeof(v6only)) < 0) {
    perror("setsockopt IPV6_V6ONLY");
    exit(EXIT_FAILURE);
  }

  // type of socket created
  memset(&address6, 0, sizeof(address6));
  address6.sin6_family = AF_INET6;
  address6.sin6_port = htons(args.mng_port);
  if (inet_pton(AF_INET6, args.mng_addr, &address6.sin6_addr) <= 0) {
    // default to any if provided addr is not valid IPv6 (e.g., IPv4 string)
    address6.sin6_addr = in6addr_any;
  }

  // bind the socket to the port
  if (bind(monitor_socket, (struct sockaddr *)&address6, sizeof(address6)) <
      0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(monitor_socket, LISTEN_BACKLOG) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  if ((error = selector_register(
           fds, monitor_socket, get_passive_monitor_handler(),
           get_passive_monitor_interests(), NULL)) != SELECTOR_SUCCESS) {
    log_to_stdout("%s\n", selector_error(error));
  }

  // initialize my logger with my selector
  logger_initialize(fds, log_file);

  if (user_init_status != USERS_OK) {
    log_to_stdout(
        "Warning: Failed to initialize users. Authentication may not work.\n");
  } else {
    log_to_stdout("Users loaded successfully from %s\n",
                  DEFAULT_USERS_FILE_PATH);
  }

  log_to_stdout("Listener on port %d\n", args.mng_port);

  while (running) {
    selector_select(fds);
  }

  log_to_stdout("\nShutting down server...\n");

  users_shutdown();
  logger_destroy();
  close(master_socket);
  selector_destroy(fds);
  selector_close();

  return 0;
}
