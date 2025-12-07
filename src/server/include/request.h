#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "../../lib/selector/selector.h"

void request_read_init(const unsigned state, struct selector_key *key);
unsigned request_read(struct selector_key *key);
void dns_lookup(const unsigned state, struct selector_key *key);
void try_connect(const unsigned state, struct selector_key *key);
unsigned request_write(struct selector_key *key);

#endif