#include "closing.h"
#include "../lib/logger/logger.h"
#include "client_utils.h"

void end_connection(const unsigned state, struct selector_key *key) {
  client_t *client = ATTACHMENT(key);

  if (client->client_closed && client->dest_closed &&
      !buffer_can_read(&client->client_buffer) &&
      !buffer_can_read(&client->destiny_buffer)) {
    printf("cerrando conexion en el socket pair: %d - %d\n", client->client_fd,
           client->destination_fd);

    destroy_active_parser(client);
    free_dns_request(client);
    free_destination(client);

    if (client->client_fd != -1) {
      close(client->client_fd);
      selector_unregister_fd(key->s, client->client_fd);
      client->client_fd = -1;
    }

    if (client->destination_fd != -1) {
      close(client->destination_fd);
      selector_unregister_fd(key->s, client->destination_fd);
      client->destination_fd = -1;
    }

    free(key->data);
  }
}

void error_handler(const unsigned state, struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  if ((state == REQUEST_READ || state == REQUEST_WRITE || state == DNS_LOOKUP ||
       state == DEST_CONNECT) &&
      client->err != 0) {
    uint8_t reply[10] = {
        0x05,                          // VER
        client->err,                   // REP (error code)
        0x00,                          // RSV
        0x01,                          // ATYP = IPv4
        0x00,        0x00, 0x00, 0x00, // BND.ADDR = 0.0.0.0
        0x00,        0x00              // BND.PORT = 0
    };

    if (send(key->fd, reply, sizeof(reply), 0) != sizeof(reply)) {
      log_to_stdout(
          "Error: could not send error message to client in socket: %d",
          client->client_fd);
    }
  }
  destroy_active_parser(client);
  free_dns_request(client);
  free_destination(client);
  client->client_closed = true;
  client->dest_closed = true;
  client->client_buffer.read = client->client_buffer.write;
  client->destiny_buffer.read = client->destiny_buffer.write;
  end_connection(state, key);
}
