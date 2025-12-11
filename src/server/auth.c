#include "include/auth.h"
#include "../lib/logger/logger.h"
#include "../lib/selector/selector.h"
#include "include/auth_utils.h"
#include "include/client_utils.h"

#define AUTH_VERSION 0x01
#define AUTH_REPLY_SIZE 2

#define AUTH_SUCCESS 0x00
#define AUTH_FAILURE 0x01

static void act_auth_ver(struct parser_event *ret, const uint8_t c) {
  ret->type = AUTH_EVENT_VER;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_auth_ulen(struct parser_event *ret, const uint8_t c) {
  ret->type = AUTH_EVENT_ULEN;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_auth_uname(struct parser_event *ret, const uint8_t c) {
  ret->type = AUTH_EVENT_UNAME;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_auth_plen(struct parser_event *ret, const uint8_t c) {
  ret->type = AUTH_EVENT_PLEN;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_auth_passwd(struct parser_event *ret, const uint8_t c) {
  ret->type = AUTH_EVENT_PASSWD;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_auth_done(struct parser_event *ret, const uint8_t c) {
  (void)c;
  ret->type = AUTH_EVENT_DONE;
  ret->n = 0;
}

static struct parser_state_transition ST_VER[] = {
    {ANY, AUTH_STATE_ULEN, act_auth_ver, NULL}};

static struct parser_state_transition ST_ULEN[] = {
    {ANY, AUTH_STATE_UNAME, act_auth_ulen, NULL}};

static struct parser_state_transition ST_UNAME[] = {
    {ANY, AUTH_STATE_UNAME, act_auth_uname, NULL}};

static struct parser_state_transition ST_PLEN[] = {
    {ANY, AUTH_STATE_PASSWD, act_auth_plen, NULL}};

static struct parser_state_transition ST_PASSWD[] = {
    {ANY, AUTH_STATE_PASSWD, act_auth_passwd, NULL}};

static struct parser_state_transition ST_FIN[] = {
    {ANY, AUTH_STATE_FIN, act_auth_done, NULL}};

static struct parser_state_transition ST_ERROR[] = {
    {ANY, AUTH_STATE_FIN, act_auth_done, NULL}};

static const struct parser_state_transition *states[] = {
    ST_VER, ST_ULEN, ST_UNAME, ST_PLEN, ST_PASSWD, ST_FIN, ST_ERROR};

static const size_t states_n[] = {sizeof(ST_VER) / sizeof(ST_VER[0]),
                                  sizeof(ST_ULEN) / sizeof(ST_ULEN[0]),
                                  sizeof(ST_UNAME) / sizeof(ST_UNAME[0]),
                                  sizeof(ST_PLEN) / sizeof(ST_PLEN[0]),
                                  sizeof(ST_PASSWD) / sizeof(ST_PASSWD[0]),
                                  sizeof(ST_FIN) / sizeof(ST_FIN[0]),
                                  sizeof(ST_ERROR) / sizeof(ST_ERROR[0])};

static const struct parser_definition auth_parser_def = {.states_count = 7,
                                                         .states = states,
                                                         .states_n = states_n,
                                                         .start_state =
                                                             AUTH_STATE_VER};

auth_status_t auth_parser_init(auth_parser_t *ap) {
  memset(ap, 0, sizeof(auth_parser_t));
  ap->p = parser_init(parser_no_classes(), &auth_parser_def);
  if (ap->p == NULL) {
    return AUTH_ERROR;
  }
  return AUTH_OK;
}

void auth_read_init(const unsigned state, struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  destroy_active_parser(client);
  auth_status_t status = auth_parser_init(&client->parser.auth_parser);
  client->active_parser = AUTH_PARSER;
  if (status != AUTH_OK) {
    error_handler(state, key);
  }
  if ((status = change_auth_methods(key, NULL)) != AUTH_OK) {
    error_handler(state, key);
  }
}

unsigned auth_read(struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  auth_parser_t *ap = &client->parser.auth_parser;

  uint8_t c;
  ssize_t n = recv(key->fd, &c, 1, 0);
  if (n <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return AUTH_READ;
    }
    return ERROR;
  }

  const struct parser_event *ev = parser_feed(ap->p, c);

  for (; ev != NULL; ev = ev->next) {
    switch (ev->type) {

    case AUTH_EVENT_VER:
      ap->ver = ev->data[0];
      log_to_stdout("Authentication version: %d\n", ap->ver);
      if (ap->ver != AUTH_VERSION) {
        return ERROR;
      }
      break;

    case AUTH_EVENT_ULEN:
      ap->ulen = ev->data[0];
      log_to_stdout("Username length: %d\n", ap->ulen);
      ap->uname_read = 0;
      break;

    case AUTH_EVENT_UNAME:
      if (ap->uname_read < ap->ulen) {
        ap->uname[ap->uname_read++] = ev->data[0];
        ap->uname[ap->uname_read] = '\0';
      }
      if (ap->uname_read == ap->ulen) {
        parser_set_state(ap->p, AUTH_STATE_PLEN);
      }
      break;

    case AUTH_EVENT_PLEN:
      ap->plen = ev->data[0];
      ap->passwd_read = 0;
      break;

    case AUTH_EVENT_PASSWD:
      if (ap->passwd_read < ap->plen) {
        ap->passwd[ap->passwd_read++] = ev->data[0];
        ap->passwd[ap->passwd_read] = '\0';
      }
      if (ap->passwd_read == ap->plen && !ap->auth_done) {
        access_level_t access_level;
        log_to_stdout("Username and password combination: %s and %s\n",
                      ap->uname, ap->passwd);
        if (user_login((char *)ap->uname, (char *)ap->passwd, &access_level) ==
            USERS_OK) {
          ap->auth_status = AUTH_SUCCESS;
        } else {
          ap->auth_status = AUTH_FAILURE;
        }
        ap->auth_done = true;
        selector_set_interest_key(key, OP_WRITE);
        return AUTH_WRITE;
      }
      break;

    default:
      return ERROR;
    }
  }
  return AUTH_READ;
}

unsigned auth_write(struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  auth_parser_t *ap = &client->parser.auth_parser;
  uint8_t response[AUTH_REPLY_SIZE];
  response[0] = AUTH_VERSION;
  response[1] = ap->auth_status;

  ssize_t n = send(key->fd, response, sizeof(response), 0);
  if (n != sizeof(response) || ap->auth_status == AUTH_FAILURE) {
    log_to_stdout("Authentication failed.\n");
    selector_set_interest_key(key, OP_NOOP);
    return ERROR;
  } else if (ap->auth_status == AUTH_SUCCESS) {
    log_to_stdout("Passed authentication!\n");
    selector_set_interest_key(key, OP_READ);
    return REQUEST_READ;
  }
  return ERROR;
}
