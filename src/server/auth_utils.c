#include "include/auth_utils.h"
#include "../../lib/logger/logger.h"
#include "../include/client_utils.h"
#include "../lib/selector/selector.h"
#include <stdbool.h>

#define DEFAULT_METHOD HELLO_AUTH_USER_PASS

static int8_t accepted_methods[MAX_BUFFER_SIZE];

static bool supported_method(int8_t method) {
  if (method != HELLO_AUTH_NO_AUTH && method != HELLO_AUTH_USER_PASS) {
    return false;
  }
  return true;
}

auth_status_t change_auth_methods(uint8_t **new_methods) {
  uint8_t i = 0;
  if (new_methods == NULL) {
    accepted_methods[i++] = DEFAULT_METHOD;
    accepted_methods[i] = -1;
  } else {
    uint8_t *current = new_methods[i];
    while (current != NULL) {
      if (!supported_method(*current)) {
        log_to_stdout("Method %d not supported\n", *current);
        return AUTH_ERROR;
      }
      accepted_methods[i] = *current;
      log_to_stdout("Added method %d for authentication\n", *current);
      current = new_methods[++i];
    }
    accepted_methods[i] = -1;
  }
  return AUTH_OK;
}

auth_status_t accepted_method(int8_t method) {
  if (method < 0) {
    return false;
  }
  int8_t i = 0;
  bool found = false;
  int8_t current = accepted_methods[i++];
  while (current != -1 && !found) {
    if (method == current) {
      found = true;
    }
    current = accepted_methods[i++];
  }
  return found;
}
