#ifndef _CLIENT_SOCKET_UTILS_
#define _CLIENT_SOCKET_UTILS_

#include "../selector/selector.h"

const int CLIENT_INTERESTS = OP_READ;

const fd_handler *get_client_handler();

#endif
