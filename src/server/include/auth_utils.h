#ifndef _AUTH_UTILS_H_
#define _AUTH_UTILS_H_

#include "auth.h"

auth_status_t change_auth_methods(struct selector_key *key,
                                  uint8_t **new_methods);

#endif