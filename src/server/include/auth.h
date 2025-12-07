#ifndef _AUTH_H_
#define _AUTH_H_

#include "../../lib/selector/selector.h"

typedef enum { AUTH } auth_state_t;

unsigned auth_read(struct selector_key *key);

unsigned auth_write(struct selector_key *key);

#endif