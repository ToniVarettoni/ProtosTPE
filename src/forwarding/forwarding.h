#ifndef _FORWARDING_H_
#define _FORWARDING_H_

void write_forward(const unsigned state, struct selector_key *key);
void read_forward(const unsigned state, struct selector_key *key);
void setup_forward(const unsigned state, struct selector_key *key);
void close_forward(const unsigned state, struct selector_key *key);

#endif