#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include "io_utils.h"
#include <stddef.h>

int send_all(int fd, const void *buf, size_t len) {

  const char *p = buf;
  int left = len;
  while (left) {
    ssize_t n = send(fd, p, left, 0);
    if (n > 0) {
      p += n;
      left -= n;
      continue;
    }
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      return len - left;
    if (n < 0 && errno == EINTR)
      continue;
    return -1;
  }
  return len;
}
