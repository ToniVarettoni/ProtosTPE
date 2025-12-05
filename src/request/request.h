#ifndef _REQUEST_H_
#define _REQUEST_H_

void read_request(const unsigned state, struct selector_key *key);
void dns_lookup(const unsigned state, struct selector_key *key);
void try_connect(const unsigned state, struct selector_key *key);
void write_request(const unsigned state, struct selector_key *key);

#endif