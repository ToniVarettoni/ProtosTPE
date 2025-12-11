#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "../selector/selector.h"
#include "logger.h"

static char *buffer = NULL;
static size_t size = 0;
static fd_selector selector = NULL;

#define MAX_LOG_FILE_PATH_LENGTH 255

static char log_file_path[MAX_LOG_FILE_PATH_LENGTH];

#define MAX_DATE_TIME_STRING_LENGTH 20
#define APPEND_MODE "a"

static int log_fd = -1;
static int log_file_fd = -1;

static char *file_buffer = NULL;
static size_t file_size = 0;

static void get_datetime(char *buffer, size_t size) {
  time_t now = time(NULL);
  struct tm tm_now;

  localtime_r(&now, &tm_now);

  strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &tm_now);
}

static char *prepend_datetime_to_lines(const char *input, size_t in_size,
                                       size_t *out_size) {
  const char *p = input;
  const char *end = input + in_size;

  size_t max_extra = 20 * (in_size / 2 + 2);
  size_t alloc_size = in_size + max_extra;

  char *out = malloc(alloc_size);
  if (!out)
    return NULL;

  char *w = out;

  while (p < end) {
    char dt[MAX_DATE_TIME_STRING_LENGTH];
    get_datetime(dt, sizeof(dt));

    size_t dt_len = strlen(dt);
    memcpy(w, dt, dt_len);
    w += dt_len;
    *w++ = ' ';

    while (p < end && *p != '\n') {
      *w++ = *p++;
    }

    if (p < end && *p == '\n') {
      *w++ = *p++;
    }
  }

  *out_size = (size_t)(w - out);
  return out;
}

// esta seria la funcion que en realidad escribe a STDOUT, manejada por el
// selector
static void logger_handle_write(struct selector_key *key) {
  if (buffer == NULL || size == 0) {
    selector_set_interest_key(key, OP_NOOP);
    return;
  }

  size_t dated_size = 0;
  char *dated = prepend_datetime_to_lines(buffer, size, &dated_size);

  if (dated == NULL) {
    perror("prepend_datetime_to_lines");
    return;
  }

  if (log_file_fd != -1) {
    char *tmp_file = realloc(file_buffer, file_size + dated_size);
    if (tmp_file != NULL) {
      file_buffer = tmp_file;
      memcpy(file_buffer + file_size, dated, dated_size);
      file_size += dated_size;
      selector_set_interest(selector, log_file_fd, OP_WRITE);
    }
  }

  ssize_t written = write(key->fd, dated, dated_size);

  if (written > 0) {
    if ((size_t)written == dated_size) {
      free(buffer);
      buffer = NULL;
      size = 0;

      selector_set_interest_key(key, OP_NOOP);
    } else {
      size_t remain = dated_size - (size_t)written;

      memmove(buffer, buffer + (size - remain), remain);
      size = remain;
    }
  } else if (written == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    // no se pudo escribir, intento despues
  } else {
    perror("write");
  }

  free(dated);
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

// escribe de manera no bloqueante en archivo
static void logger_file_handle_write(struct selector_key *key) {
  if (file_buffer == NULL || file_size == 0) {
    selector_set_interest_key(key, OP_NOOP);
    return;
  }

  ssize_t written = write(key->fd, file_buffer, file_size);

  if (written > 0) {
    if ((size_t)written == file_size) {
      free(file_buffer);
      file_buffer = NULL;
      file_size = 0;

      selector_set_interest_key(key, OP_NOOP);
    } else {
      size_t remain = file_size - (size_t)written;
      memmove(file_buffer, file_buffer + written, remain);
      file_size = remain;
    }
  } else if (written == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    // no pude escribir, intento despues
  } else {
    perror("write to log file");
  }
}

static void logger_file_handle_close(struct selector_key *key) {
  free(file_buffer);
  file_buffer = NULL;
  file_size = 0;
}

static fd_handler logger_file_handler = {
    .handle_write = logger_file_handle_write,
    .handle_close = logger_file_handle_close,
    .handle_read = NULL,
    .handle_block = NULL,
};

void logger_initialize(fd_selector selector_param, char *file_path) {
  // duplico stdout para poder registrar y despues desregistrar una copia del FD
  // en vez de usarlo directamente
  int fd = dup(STDOUT_FILENO);

  // hago que toda escritura a este nuevo FD sea no bloqueante
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  log_fd = fd;
  selector = selector_param;

  strcpy(log_file_path, file_path);

  // abro el archivo con flags no bloqueantes
  log_file_fd = open(log_file_path, O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK, 0644);
  if (log_file_fd == -1) {
    perror("open log file");
  }

  selector_register(selector, fd, &logger_handler, OP_NOOP, NULL);
  
  if (log_file_fd != -1) {
    selector_register(selector, log_file_fd, &logger_file_handler, OP_NOOP, NULL);
  }
}

void log_to_stdout(char *format, ...) {
  va_list args;

  va_start(args, format);
  // esto me va a devolver el tamanio del string pedido a loggear
  int real_size = vsnprintf(NULL, 0, format, args);
  va_end(args);

  if (real_size < 0)
    return;

  size_t new_size = size + (size_t)real_size;
  char *tmp = realloc(buffer, new_size + 1);
  if (tmp == NULL) {
    perror("realloc");
    free(buffer);
    buffer = NULL;
    size = 0;
    return;
  }
  buffer = tmp;

  va_start(args, format);
  // aca si me encargo de cargarle al buffer el string pedido
  vsnprintf(buffer + size, (size_t)real_size + 1, format, args);
  va_end(args);

  size = new_size;

  selector_set_interest(selector, log_fd, OP_WRITE);
}

void logger_flush(void) {
  while (buffer != NULL && size > 0 && selector != NULL) {
    selector_select(selector);
  }
}

void logger_destroy() {
  if (log_fd != -1) {
    close(log_fd);
    selector_unregister_fd(selector, log_fd);
  }
  log_fd = -1;
  
  if (log_file_fd != -1) {
    close(log_file_fd);
    selector_unregister_fd(selector, log_file_fd);
  }
  log_file_fd = -1;
}
