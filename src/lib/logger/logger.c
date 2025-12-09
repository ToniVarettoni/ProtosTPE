#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../selector/selector.h"
#include "logger.h"

static char *buffer = NULL;
static size_t size = 0;
static fd_selector selector = NULL;

static fd_selector selector;

static int log_fd = -1;

// esta seria la funcion que en realidad escribe a STDOUT, manejada por el
// selector
static void logger_handle_write(struct selector_key *key) {
  if (buffer == NULL || size == 0) {
    selector_set_interest_key(key, OP_NOOP);
    return;
  }

  ssize_t written = write(key->fd, buffer, size);
  if (written > 0) {
    if (written == size) {
      // me aseguro de reiniciar mi buffer antes de otro log y setteo mi interes
      free(buffer);
      buffer = NULL;
      size = 0;
      selector_set_interest_key(key, OP_NOOP);
    } else if (written < size) { // manejo de write parcial
      memmove(buffer, buffer + written, size - written);
      size -= written;
    }

  } else if (written == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    return; // retorno dejando mi interes como para escribir
  }
}

// libera y reinicia los recursos relevantes al logger con la excepcion del FD
// duplicado y el selector_unregister
static void logger_handle_close(struct selector_key *key) {
  free(buffer);
  buffer = NULL;
  size = 0;
}

static fd_handler logger_handler = {
    .handle_write = logger_handle_write,
    .handle_close = logger_handle_close,
    .handle_read = NULL,
    .handle_block = NULL,
};

void logger_initialize(fd_selector selector_param) {
  // duplico stdout para poder registrar y despues desregistrar una copia del FD
  // en vez de usarlo directamente
  int fd = dup(STDOUT_FILENO);

  // hago que toda escritura a este nuevo FD sea no bloqueante
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  log_fd = fd;
  selector = selector_param;

  selector_register(selector, fd, &logger_handler, OP_NOOP, NULL);
}

void log_to_stdout(char *format, ...) {
  va_list args;

  va_start(args, format);
  int real_size = vsnprintf(NULL, 0, format, args);
  va_end(args);

  if (real_size < 0)
    return;

  buffer = (char *)malloc(real_size + 1);
  size = real_size;

  va_start(args, format);
  vsnprintf(buffer, real_size + 1, format, args);
  va_end(args);

  selector_set_interest(selector, log_fd, OP_WRITE);
}

void logger_destroy() {
  if (log_fd != -1) {
    close(log_fd);
    selector_unregister_fd(selector, log_fd);
  }
  log_fd = -1;
}
