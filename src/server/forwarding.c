#include "forwarding.h"

unsigned forward_write(struct selector_key *key) { return 0; }

unsigned forward_read(struct selector_key *key) { return 0; }

void forward_setup(const unsigned state, struct selector_key *key) {}

void forward_close(const unsigned state, struct selector_key *key) {}