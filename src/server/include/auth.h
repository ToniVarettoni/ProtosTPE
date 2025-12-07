#ifndef _AUTH_H_
#define _AUTH_H_

#include "../../lib/selector/selector.h"

unsigned auth_read(struct selector_key *key);
unsigned auth_write(struct selector_key *key);

#endif