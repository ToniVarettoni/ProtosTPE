#ifndef _AUTH_H_
#define _AUTH_H_

#include "../../lib/selector/selector.h"

void auth_read_init(const unsigned state, struct selector_key *key);
unsigned auth_read(struct selector_key *key);
unsigned auth_write(struct selector_key *key);

#endif