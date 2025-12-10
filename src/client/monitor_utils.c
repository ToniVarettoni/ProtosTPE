#include "include/monitor_utils.h"
#include <string.h>

size_t build_monitor_auth_request(uint8_t *buffer, size_t buffer_len,
                                  const user_t *login_user) {
  if (buffer == NULL || login_user == NULL || login_user->username == NULL ||
      login_user->password == NULL) {
    return 0;
  }

  size_t ulen = strlen(login_user->username);
  size_t plen = strlen(login_user->password);

  if (ulen == 0 || ulen > UINT8_MAX || plen == 0 || plen > UINT8_MAX) {
    return 0;
  }

  size_t needed = 2 + ulen + plen;
  if (needed > buffer_len) {
    return 0;
  }

  size_t pos = 0;
  buffer[pos++] = (uint8_t)ulen;
  memcpy(buffer + pos, login_user->username, ulen);
  pos += ulen;
  buffer[pos++] = (uint8_t)plen;
  memcpy(buffer + pos, login_user->password, plen);
  pos += plen;

  return pos;
}
