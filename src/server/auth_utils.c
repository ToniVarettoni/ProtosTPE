#include "include/auth_utils.h"
#include "../../lib/logger/logger.h"
#include "../include/client_utils.h"
#include "../lib/selector/selector.h"
#include <stdbool.h>

#define DEFAULT_METHOD HELLO_AUTH_USER_PASS

static bool supported_method(int8_t method) {
  if (method != HELLO_AUTH_NO_AUTH && method != HELLO_AUTH_USER_PASS) {
    return false;
  }
  return true;
}

auth_status_t change_auth_methods(struct selector_key *key,
                                  uint8_t **new_methods) {
  auth_parser_t *ap = &((client_t *)ATTACHMENT(key))->parser.auth_parser;
  uint8_t i = 0;
  if (new_methods == NULL) {
    ap->methods_accepted[i++] = DEFAULT_METHOD;
    ap->methods_accepted[i] = -1;
  } else {
    uint8_t *current = new_methods[i];
    while (current != NULL) {
      if (!supported_method(*current)) {
        log_to_stdout("Method %d not supported\n", *current);
        return AUTH_ERROR;
      }
      ap->methods_accepted[i] = *current;
      current = new_methods[++i];
    }
    ap->methods_accepted[i] = -1;
  }
  return AUTH_OK;
}
