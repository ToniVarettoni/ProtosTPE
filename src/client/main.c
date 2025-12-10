#include "../lib/args_monitor/args_monitor.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "include/monitor_utils.h"

static int create_monitor_socket(const char *direction, unsigned short port) {
  if (direction != NULL && strcmp(direction, "localhost") == 0) {
    direction = "127.0.0.1";
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_pton(AF_INET, direction, &addr.sin_addr) != 1) {
    fprintf(stderr, "Invalid address for monitor: %s\n", direction);
    return -1;
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "socket failed: %s\n", strerror(errno));
    return -1;
  }

  if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "connect failed: %s\n", strerror(errno));
    close(sockfd);
    return -1;
  }

  return sockfd;
}

int main(int argc, char **argv) {
  management_args_t args;
  parse_monitor_args(argc, argv, &args);

  printf("Connecting to %s:%hu\n", args.direction, args.port);
  int sockfd = create_monitor_socket(args.direction, args.port);
  if (sockfd < 0) {
    return 1;
  }
  printf("Client connected to monitor!\n");

  uint8_t auth_buf[512];
  size_t auth_len =
      build_monitor_auth_request(auth_buf, sizeof(auth_buf), &args.managing_user);
  if (auth_len == 0) {
    fprintf(stderr, "Failed to build auth request.\n");
    close(sockfd);
    return 1;
  }

  ssize_t sent = send(sockfd, auth_buf, auth_len, 0);
  if (sent < 0) {
    fprintf(stderr, "Failed to send auth request: %s\n", strerror(errno));
    close(sockfd);
    return 1;
  }
  printf("Sent auth request (%zd bytes)\n", sent);

  return 0;
}
