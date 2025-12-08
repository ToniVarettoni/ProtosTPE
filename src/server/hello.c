#include "../include/hello.h"

#define HELLO_PROTOCOL_VERSION 0x05
#define HELLO_NO_ACCEPTABLE_METHODS 0xFF
#define HELLO_REPLY_SIZE 2

static void act_ver(struct parser_event *ret, const uint8_t c) {
  ret->type = HELLO_EVENT_VER;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_nmethods(struct parser_event *ret, const uint8_t c) {
  ret->type = HELLO_EVENT_NMETHODS;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_method(struct parser_event *ret, const uint8_t c) {
  ret->type = HELLO_EVENT_METHOD;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_done(struct parser_event *ret, const uint8_t c) {
  (void)c;
  ret->type = HELLO_EVENT_DONE;
  ret->n = 0;
}

static struct parser_state_transition ST_VER[] = {
    {ANY, HELLO_STATE_NMETHODS, act_ver, NULL}};

static struct parser_state_transition ST_NMETHODS[] = {
    {ANY, HELLO_STATE_METHODS, act_nmethods, NULL}};

static struct parser_state_transition ST_METHODS[] = {
    {ANY, HELLO_STATE_METHODS, act_method, NULL}};

static struct parser_state_transition ST_FIN[] = {
    {ANY, HELLO_STATE_FIN, act_done, NULL}};

static struct parser_state_transition ST_ERR[] = {
    {ANY, HELLO_STATE_ERROR, act_done, NULL}};

static const struct parser_state_transition *states[] = {
    ST_VER, ST_NMETHODS, ST_METHODS, ST_FIN, ST_ERR};

static const size_t states_n[] = {
    sizeof(ST_VER) / sizeof(ST_VER[0]),
    sizeof(ST_NMETHODS) / sizeof(ST_NMETHODS[0]),
    sizeof(ST_METHODS) / sizeof(ST_METHODS[0]),
    sizeof(ST_FIN) / sizeof(ST_FIN[0]),
    sizeof(ST_ERR) / sizeof(ST_ERR[0]),
};

static const struct parser_definition hello_parser_def = {
    .states_count = 5,
    .states = states,
    .states_n = states_n,
    .start_state = HELLO_STATE_VER,
};

hello_status_t hello_init(struct hello_parser *hp) {
  memset(hp, 0, sizeof(struct hello_parser));
  hp->p = parser_init(parser_no_classes(), &hello_parser_def);
  if (hp->p == NULL) {
    return HELLO_UNKNOWN_ERROR;
  }
  return HELLO_OK;
}

hello_status_t hello_read(struct selector_key *key) {
  struct hello_parser *hp = ATTACHMENT(key);

  uint8_t c;
  ssize_t left = recv(key->fd, &c, MAX_BUFFER, 0);
  if (left <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return HELLO_OK;
    }
    return HELLO_UNKNOWN_ERROR;
  }
  while (left) {
    const struct parser_event *ev = parser_feed(hp->p, c);

    for (; ev != NULL; ev = ev->next) {
      switch (ev->type) {

      case HELLO_EVENT_VER:
        hp->ver = ev->data[0];
        if (hp->ver != HELLO_PROTOCOL_VERSION) {
          return HELLO_VER_ERROR;
        }
        break;

      case HELLO_EVENT_NMETHODS:
        if (hp->nmethods == 0) {
          return HELLO_NMETHODS_ERROR;
        }
        hp->nmethods = ev->data[0];
        hp->methods_read = 0;
        break;

      case HELLO_EVENT_METHOD:
        if (hp->methods_read < hp->nmethods) {
          hp->methods[hp->methods_read++] = ev->data[0];
        }
        if (hp->methods_read == hp->nmethods) {
          bool found = false;
          hp->method_selected = HELLO_AUTH_NO_METHOD_ACCEPTED;

          for (int i = 0; i < hp->nmethods && !found; i++) {
            if (hp->methods[i] == HELLO_AUTH_USER_PASS) {
              hp->method_selected = HELLO_AUTH_USER_PASS;
              found = true;
              break;
            }
          }

          if (!found) {
            hp->method_selected = HELLO_NO_ACCEPTABLE_METHODS;
          }

          parser_reset(hp->p);
          return HELLO_OK;
        }
        break;

      case HELLO_EVENT_UNEXPECTED:
        return HELLO_UNKNOWN_ERROR;

      case HELLO_EVENT_DONE:
        break;

      default:
        return HELLO_UNKNOWN_ERROR;
      }
    }
    left--;
  }

  return HELLO_OK;
}

hello_status_t hello_write(struct selector_key *key) {
  struct hello_parser *hp = ATTACHMENT(key);
  uint8_t reply[HELLO_REPLY_SIZE] = {HELLO_PROTOCOL_VERSION, hp->method_selected};

  if (send(key->fd, reply, HELLO_REPLY_SIZE, 0) != HELLO_REPLY_SIZE) {
    return HELLO_UNKNOWN_ERROR;
  }

  return HELLO_OK;
}
