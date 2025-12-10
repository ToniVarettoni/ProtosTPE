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
size_t build_monitor_auth_request(uint8_t *buffer, size_t buffer_len,
                                  const user_t *login_user);

#endif
