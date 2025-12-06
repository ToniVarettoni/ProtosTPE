#ifndef _SETUP_H_
#define _SETUP_H_

#include "../../lib/selector/selector.h"

unsigned read_hello(struct selector_key *key);
unsigned write_hello(struct selector_key *key);
unsigned read_auth(struct selector_key *key);
unsigned write_auth(struct selector_key *key);

#endif