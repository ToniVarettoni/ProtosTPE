#include "forwarding.h"
#include "../lib/logger/logger.h"
#include "stats.h"
#include "client_utils.h"


unsigned forward_write(struct selector_key *key) {    
    client_t *client = ATTACHMENT(key);
    buffer * b = (key->fd == client->client_fd)? &client->destiny_buffer : &client->client_buffer;
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
    log_to_stdout("Successfully sent %d bytes to client %d\n", n, key->fd);

    if (buffer_can_read(b)){
        selector_set_interest(key->s, key->fd, OP_READ | OP_WRITE);
    }else{
        selector_set_interest(key->s, key->fd, OP_READ);
    }
    return FORWARDING;
}

unsigned forward_read(struct selector_key *key) {

    client_t *client = ATTACHMENT(key);
    buffer * b = (key->fd == client->client_fd)? &client->client_buffer : &client->destiny_buffer;
    
    if (!buffer_can_write(b)) {
        selector_set_interest(key->s, key->fd, OP_READ | OP_WRITE);
        return FORWARDING;
    }
    
    size_t nbytes;
    uint8_t * write_ptr = buffer_write_ptr(b, &nbytes);
    errno = 0;
    size_t n = recv(key->fd, write_ptr, nbytes, 0);
    if (n <= 0) {
        if (errno == 0 && n == 0){
            printf("Client %d closed connection.\n", key->fd);
            return DONE;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            errno = 0;
            return FORWARDING;
        }
        log_to_stdout("Error reading from client.\n");
        selector_set_interest_key(key, OP_NOOP);
        return ERROR;
    }
    log_to_stdout("Succesfully read %d bytes from client: %d\n", n, key->fd);
    buffer_write_adv(b, n);

    selector_set_interest(key->s, (key->fd == client->client_fd)? client->destination_fd : client->client_fd, OP_READ | OP_WRITE);
    
    return FORWARDING;
    }

void forward_setup(const unsigned state, struct selector_key *key) {}

void forward_close(const unsigned state, struct selector_key *key) {}
