#ifndef _AUTH_UTILS_H_
#define _AUTH_UTILS_H_

#include "auth.h"

auth_status_t change_auth_methods(uint8_t **new_methods);

auth_status_t accepted_method(int8_t method);

#endif