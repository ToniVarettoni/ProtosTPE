#include "monitor_auth.h"
#include "../../lib/parser/parser.h"
#include "../include/client_utils.h"
#include "monitor.h"

static void act_monitor_auth_ulen(struct parser_event *ret, const uint8_t c) {
  ret->type = MONITOR_AUTH_EVENT_ULEN;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_monitor_auth_uname(struct parser_event *ret, const uint8_t c) {
  ret->type = MONITOR_AUTH_EVENT_UNAME;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_monitor_auth_plen(struct parser_event *ret, const uint8_t c) {
  ret->type = MONITOR_AUTH_EVENT_PLEN;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_monitor_auth_password(struct parser_event *ret,
                                      const uint8_t c) {
  ret->type = MONITOR_AUTH_EVENT_PASSWD;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_monitor_auth_done(struct parser_event *ret, const uint8_t c) {
  ret->type = MONITOR_AUTH_EVENT_DONE;
  ret->n = 0;
}

static struct parser_state_transition ST_ULEN[] = {
    {ANY, MONITOR_AUTH_STATE_UNAME, act_monitor_auth_ulen, NULL},
};

static struct parser_state_transition ST_UNAME[] = {
    {ANY, MONITOR_AUTH_STATE_PLEN, act_monitor_auth_uname, NULL},
};

static struct parser_state_transition ST_PLEN[] = {
    {ANY, MONITOR_AUTH_STATE_PASSWD, act_monitor_auth_plen, NULL},
};

static struct parser_state_transition ST_PASSWD[] = {
    {ANY, MONITOR_AUTH_STATE_DONE, act_monitor_auth_password, NULL},
};

static struct parser_state_transition ST_DONE[] = {
    {ANY, MONITOR_AUTH_STATE_DONE, act_monitor_auth_done, NULL},
};

static struct parser_state_transition ST_ERR[] = {
    {ANY, MONITOR_AUTH_STATE_ERROR, act_monitor_auth_done, NULL}};

static const struct parser_state_transition *states[] = {
    ST_ULEN, ST_UNAME, ST_PLEN, ST_PASSWD, ST_DONE, ST_ERR};

static const size_t states_n[] = {sizeof(ST_ULEN) / sizeof(ST_ULEN[0]),
                                  sizeof(ST_UNAME) / sizeof(ST_UNAME[0]),
                                  sizeof(ST_PLEN) / sizeof(ST_PLEN[0]),
                                  sizeof(ST_PASSWD) / sizeof(ST_PASSWD[0]),
                                  sizeof(ST_DONE) / sizeof(ST_DONE[0]),
                                  sizeof(ST_ERR) / sizeof(ST_ERR[0])};

static const struct parser_definition monitor_auth_parser_def = {
    .states_count = 6,
    .states = states,
    .states_n = states_n,
    .start_state = MONITOR_AUTH_STATE_ULEN};

monitor_auth_status_t monitor_auth_parser_init(monitor_auth_parser_t *map) {
  memset(map, 0, sizeof(monitor_auth_parser_t));
  map->p = parser_init(parser_no_classes(), &monitor_auth_parser_def);
  if (map->p == NULL) {
    return MONITOR_AUTH_STATUS_ERR;
  }
  return MONITOR_AUTH_STATUS_OK;
}

void monitor_init(const unsigned state, struct selector_key *key) {
  monitor_auth_parser_t *map = ATTACHMENT(key);
  if (monitor_auth_parser_init(map) != MONITOR_AUTH_STATUS_OK) {
    close_connection(key);
  }
}

unsigned monitor_auth_read(struct selector_key *key) {
  monitor_t *monitor = ATTACHMENT(key);
  monitor_auth_parser_t *map = &monitor->parser.auth_parser;

  uint8_t c;
  ssize_t n = recv(key->fd, &c, 1, 0);
  if (n <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return HELLO_READ;
    }
    return ERROR;
  }

  const struct parser_event *ev = parser_feed(map->p, c);
  for (; ev != NULL; ev = ev->next) {
    switch (ev->type) {

    case MONITOR_AUTH_EVENT_ULEN:
      map->ulen = ev->data[0];
      map->uname_read = 0;
      break;

    case MONITOR_AUTH_EVENT_UNAME:
      if (map->uname_read < map->ulen) {
        map->uname[map->uname_read++] = ev->data[0];
        map->uname[map->uname_read] = '\0';
      }
      if (map->uname_read == map->ulen) {
        parser_set_state(map->p, MONITOR_AUTH_STATE_PLEN);
      }
      break;
    case MONITOR_AUTH_EVENT_PLEN:
      map->plen = ev->data[0];
      map->passwd_read = 0;
      break;
    case MONITOR_AUTH_EVENT_PASSWD:
      if (map->passwd_read < map->plen) {
        map->passwd[map->passwd_read++] = ev->data[0];
        map->passwd[map->passwd_read] = '\0';
      }
      if (map->passwd_read == map->plen) {
        if (user_login(map->uname, map->passwd, &monitor->user_access_level) !=
            USERS_OK) {
          return MONITOR_ERROR;
        }
        parser_set_state(map->p, MONITOR_AUTH_STATE_DONE);
      }
    case MONITOR_AUTH_EVENT_DONE:
      return MONITOR_REQ_READ;
    default:
      return MONITOR_ERROR;
    }
  }
  return MONITOR_ERROR;
}