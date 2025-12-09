#include "../lib/logger/logger.h"
#include "../lib/selector/selector.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_FDS 10

int main() {
  const char *username = "alice";
  const char *password = "secret";

  uint8_t ulen = (uint8_t)strlen(username);
  uint8_t plen = (uint8_t)strlen(password);

  // Crear socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    return 1;
  }

  // Configurar destino: localhost:8080
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

  // Conectar
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("connect");
    close(sock);
    return 1;
  }

  // Armar mensaje
  uint8_t buffer[512];
  size_t pos = 0;

  buffer[pos++] = ulen; // longitud username
  memcpy(&buffer[pos], username, ulen);
  pos += ulen;

  buffer[pos++] = plen; // longitud password
  memcpy(&buffer[pos], password, plen);
  pos += plen;

  // Enviar
  ssize_t sent = send(sock, buffer, pos, 0);
  if (sent < 0) {
    perror("send");
    close(sock);
    return 1;
  }

  printf("Enviado %zd bytes\n", sent);

  close(sock);
  return 0;

  // struct selector_init sel_init = {
  //     .signal = 0,
  //     .select_timeout = {.tv_sec = 1, .tv_nsec = 0},
  // };

  // selector_init(&sel_init);

  // fd_selector fds = selector_new(MAX_FDS);

  // logger_initialize(fds);

  // stats *current_stats = get_stats();

  // if (current_stats == NULL) {
  //   log_to_stdout("The server is currently down\n");

  //   logger_destroy();
  //   selector_destroy(fds);
  //   selector_close();
  //   return 1;
  // }

  // char time_str[100];
  // time_t now = time(NULL);
  // struct tm *tm_info = localtime(&now);
  // strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

  // log_to_stdout("STATISTICS FOR SERVER - %s\n\n", time_str);

  // log_to_stdout("Historic connections to the server: %d\n",
  //               current_stats->historic_connections);

  // log_to_stdout("Current connections to the server: %d\n",
  //               current_stats->current_connections);

  // log_to_stdout("Total bytes transferred from server to client: %d\n",
  //               current_stats->transferred_bytes);

  // log_to_stdout("END OF STATISTICS, EXECUTE AGAIN FOR UPDATED
  // INFORMATION\n");

  // selector_destroy(fds);

  // selector_close();

  return 0;
}