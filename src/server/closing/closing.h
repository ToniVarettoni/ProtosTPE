#ifndef _CLOSING_H_
#define _CLOSING_H_

#include "../../lib/selector/selector.h"

void end_connection(const unsigned state, struct selector_key *key);
void error_handler(const unsigned state, struct selector_key *key);

#endif