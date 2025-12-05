#ifndef _REQUEST_H_
#define _REQUEST_H_

unsigned read_request(struct selector_key *key);
void dns_lookup(const unsigned state, struct selector_key *key);
void try_connect(const unsigned state, struct selector_key *key);
unsigned write_request(struct selector_key *key);

#endif