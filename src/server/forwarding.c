#include "forwarding.h"
#include "../lib/logger/logger.h"
#include "stats.h"
#include "client_utils.h"
#include <errno.h>

static bool connection_finished(client_t *client) {
    return client->client_closed && client->dest_closed &&
           !buffer_can_read(&client->client_buffer) &&
           !buffer_can_read(&client->destiny_buffer);
}

unsigned forward_write(struct selector_key *key) {    
    client_t *client = ATTACHMENT(key);
    bool to_client = (key->fd == client->client_fd);
    buffer *b = to_client ? &client->destiny_buffer : &client->client_buffer;
    bool source_closed = to_client ? client->dest_closed : client->client_closed;

    if (!buffer_can_read(b)) {
        if (connection_finished(client)) {
            return DONE;
        }
        if (source_closed) {
            shutdown(key->fd, SHUT_WR);
        }
        selector_set_interest(key->s, key->fd, OP_READ);
        return FORWARDING;
    }

    size_t readable;
    uint8_t *read_ptr = buffer_read_ptr(b, &readable);

    ssize_t n = send(key->fd, read_ptr, readable, 0);
    if (n <= 0){
        if (errno == EAGAIN || errno == EWOULDBLOCK){
            errno = 0;
            return FORWARDING;
        }
        log_to_stdout("Error writing to client with fd: %d\n", key->fd);
        selector_set_interest_key(key, OP_NOOP);
        return ERROR;
    }
    buffer_read_adv(b, n);
    add_transferred_bytes((size_t)n);
    log_to_stdout("Successfully sent %zd bytes to client %d\n", n, key->fd);

    fd_interest interest = OP_READ;
    if (buffer_can_read(b)) {
        interest |= OP_WRITE;
    } else if (source_closed) {
        shutdown(key->fd, SHUT_WR);
    }
    selector_set_interest(key->s, key->fd, interest);

    if (connection_finished(client)) {
        return DONE;
    }

    return FORWARDING;
}

unsigned forward_read(struct selector_key *key) {

    client_t *client = ATTACHMENT(key);
    bool from_client = (key->fd == client->client_fd);

    buffer *b = from_client ? &client->client_buffer : &client->destiny_buffer;
    
    if (!buffer_can_write(b)) {
        selector_set_interest(key->s, key->fd, OP_READ | OP_WRITE);
        return FORWARDING;
    }
    
    size_t nbytes;
    uint8_t * write_ptr = buffer_write_ptr(b, &nbytes);
    errno = 0;
    ssize_t n = recv(key->fd, write_ptr, nbytes, 0);
    if (n == 0) {
            if (from_client) {
                client->client_closed = true;
                if (client->dest_closed){
                    return DONE;
                }
            } else {
                client->dest_closed = true;
                if (client->client_closed){
                    return DONE;
                }
            }
            int other_fd =
                from_client ? client->destination_fd : client->client_fd;
            if (other_fd >= 0) {
                selector_set_interest(key->s, other_fd, OP_WRITE);
            }
            selector_set_interest_key(key, OP_NOOP);
            return FORWARDING;
    }
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            errno = 0;
            return FORWARDING;
        }
        log_to_stdout("Error reading from client.\n");
        selector_set_interest_key(key, OP_NOOP);
        return ERROR;
    }
    log_to_stdout("Succesfully read %zd bytes from client: %d\n", n, key->fd);
    buffer_write_adv(b, n);

    int other_fd = (key->fd == client->client_fd) ? client->destination_fd
                                                  : client->client_fd;
    if (other_fd >= 0) {
        selector_set_interest(key->s, other_fd, OP_READ | OP_WRITE);
    }
    
    return FORWARDING;
    }

void forward_setup(const unsigned state, struct selector_key *key) {
    client_t *client = ATTACHMENT(key);
    selector_set_interest(key->s, client->client_fd, OP_READ);
    selector_set_interest(key->s, client->destination_fd, OP_READ);
}

void forward_close(const unsigned state, struct selector_key *key) {}
