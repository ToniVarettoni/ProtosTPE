#ifndef MONITOR_UTILS_H
#define MONITOR_UTILS_H

#include "../../lib/args_monitor/args_monitor.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Builds the authentication frame for the monitor protocol:
 * [ulen][uname bytes][plen][passwd bytes]
 * Returns the total frame length, or 0 on error (invalid lengths).
 */
size_t write_monitor_auth_request(uint8_t *buffer, size_t buffer_len, const user_t *login_user);

/**
 * Reads a 1-byte auth reply from the server (0x00 == OK, anything else == fail).
 * Returns true on success, false otherwise. Does not block.
 */
bool read_monitor_auth_reply(int sockfd);

#endif
