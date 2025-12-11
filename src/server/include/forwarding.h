#ifndef _FORWARDING_H_
#define _FORWARDING_H_

#include "../../lib/selector/selector.h"

unsigned forward_write(struct selector_key *key);
unsigned forward_read(struct selector_key *key);
void forward_setup(const unsigned state, struct selector_key *key);
void forward_close(const unsigned state, struct selector_key *key);
unsigned puto(struct selector_key *key);
#endif