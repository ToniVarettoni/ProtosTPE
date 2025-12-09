#ifndef _MONITOR_REQ_H_
#define _MONITOR_REQ_H_

#include "../../lib/selector/selector.h"

void monitor_read_init(const unsigned state, struct selector_key *key);

unsigned monitor_req_read(struct selector_key *key);

void monitor_req_read_finalize(const unsigned state, struct selector_key *key);

#endif