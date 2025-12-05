#ifndef _SETUP_H_
#define _SETUP_H_

void read_hello(const unsigned state, struct selector_key *key);
void write_hello(const unsigned state, struct selector_key *key);
void read_auth(const unsigned state, struct selector_key *key);
void write_auth(const unsigned state, struct selector_key *key);

#endif