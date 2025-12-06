#include "setup.h"
#include <sys/socket.h>

unsigned read_hello(struct selector_key *key) {
  // uint8_t buff[BUFFER_SIZE];
  // client_t *client = client_t * (key->data);
  // int n = recv(key->fd, buff, BUFFER_SIZE, 0);
  // int currState = client->stm->state;
  // ;
  // if (n < 0) {
  //   if (errno = EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
  //     return currState;
  //   }
  //   return -1;
  // } else {
  //   buffer_write_adv(client->reading_buffer);
  // }

  // if (buff[n] == '\r\n') {
  //   return currState + 1;
  // }
  // return currState;
  return 0;
}

unsigned write_hello(struct selector_key *key) { return 0; }

unsigned read_auth(struct selector_key *key) { return 0; }

unsigned write_auth(struct selector_key *key) { return 0; }