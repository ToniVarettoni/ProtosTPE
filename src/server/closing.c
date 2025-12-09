#include "closing.h"
#include "client_utils.h"

void end_connection(const unsigned state, struct selector_key *key) {
    client_t *client = ATTACHMENT(key);
    selector_unregister_fd(key->s, client->client_fd);
    selector_unregister_fd(key->s, client->destination_fd);
    close(client->client_fd);
    close(client->destination_fd);
    if (client->parser.hello_parser.p != NULL) {
        parser_destroy(client->parser.hello_parser.p);
    }
    if (client->parser.auth_parser.p != NULL) {
        parser_destroy(client->parser.auth_parser.p);
    }
    if (client->parser.request_parser.p != NULL) {
        parser_destroy(client->parser.request_parser.p);
    }
    free(key->data);
    free(key);
}

void error_handler(const unsigned state, struct selector_key *key) {}