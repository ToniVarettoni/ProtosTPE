#ifndef _FORWARDING_H_
#define _FORWARDING_H_

#include "../include/selector.h"

unsigned write_forward(struct selector_key *key);
unsigned read_forward(struct selector_key *key);
void setup_forward(const unsigned state, struct selector_key *key);
void close_forward(const unsigned state, struct selector_key *key);

#endif