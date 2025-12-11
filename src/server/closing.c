#include "closing.h"
#include "client_utils.h"

void end_connection(const unsigned state, struct selector_key *key) {
    client_t *client = ATTACHMENT(key);
    printf("client closed: %d\n", client->client_closed);
    printf("dest closed: %d\n", client->dest_closed);
    printf("client buffer can read: %d\n", buffer_can_read(&client->client_buffer));
    printf("dest buffer can read: %d\n", buffer_can_read(&client->destiny_buffer));
    printf("client fd: %d\n", client->client_fd);
    printf("dest fd: %d\n", client->destination_fd);
    selector_set_interest_key(key, OP_NOOP);

    if (client->client_closed && client->dest_closed && !buffer_can_read(&client->client_buffer) && !buffer_can_read(&client->destiny_buffer)){
        printf("cerrando conexion en el socket pair: %d - %d\n", client->client_fd, client->destination_fd);

        if (client->client_fd != -1){
            close(client->client_fd);
        }

        if (client->destination_fd != -1){
            close(client->destination_fd);
        }

        if (client->active_parser == HELLO_PARSER) {
            parser_destroy(client->parser.hello_parser.p);
        }

        if (client->active_parser == AUTH_PARSER) {
            parser_destroy(client->parser.auth_parser.p);
        }

        if (client->active_parser == REQUEST_PARSER) {
            parser_destroy(client->parser.request_parser.p);
        }

        selector_unregister_fd(key->s, client->client_fd);
        selector_unregister_fd(key->s, client->destination_fd);
        free(key->data);
        printf("a\n");

    }
}

void error_handler(const unsigned state, struct selector_key *key) {
    client_t * client = ATTACHMENT(key);
    if ((state == REQUEST_READ || state == REQUEST_WRITE || state == DNS_LOOKUP || state == DEST_CONNECT) && client->err != 0){
            uint8_t reply[10] = {
            0x05,                   // VER
            client->err,            // REP (error code)
            0x00,                   // RSV
            0x01,                   // ATYP = IPv4
            0x00, 0x00, 0x00, 0x00, // BND.ADDR = 0.0.0.0
            0x00, 0x00              // BND.PORT = 0
        };

        if (send(key->fd, reply, sizeof(reply), 0) != sizeof(reply)){
            log_to_stdout("Error: could not send error message to client in socket: %d", client->client_fd);
        }

    }
    
    end_connection(state, key);
}