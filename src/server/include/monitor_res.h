#ifndef _MONITOR_RES_H_
#define _MONITOR_RES_H_

#include "../include/monitor_res.h"

void monitor_res_write_init(const unsigned state, struct selector_key *key);

unsigned monitor_res_write(struct selector_key *key);

void monitor_res_write_finalize(const unsigned state, struct selector_key *key);

#endif