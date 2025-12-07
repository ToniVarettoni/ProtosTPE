#include "include/auth.h"

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

static const struct parse_state_transition *states[] = {
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

auth_status_t auth_init(struct auth_parser *ap) {
  memset(ap, 0, sizeof(struct auth_parser));
  ap->p = parser_init(parser_no_classes(), &auth_parser_def);
  if (ap->p == NULL) {
    return AUTH_ERROR;
  }
  return AUTH_OK;
}

auth_status_t auth_read(struct selector_key *key) {
  struct auth_parser *ap = ATTACHMENT(key);

  uint8_t c;
  ssize_t left = recv(key->fd, &c, MAX_BUFFER, 0);
  if (left <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return AUTH_OK;
    }
    return AUTH_ERROR;
  }
  while (left) {
    const struct parser_event *ev = parser_feed(ap->p, c);

    for (; ev != NULL; ev = ev->next) {
      switch (ev->type) {

      case AUTH_EVENT_VER:
        ap->ver = ev->data[0];
        if (ap->ver != 0x01) {
          return AUTH_ERROR;
        }
        break;

      case AUTH_EVENT_ULEN:
        ap->ulen = ev->data[0];
        ap->passwd_read = 0;
        ap->passwd[0] = '\0';
        break;

      case AUTH_EVENT_UNAME:
        if (ap->uname_read < ap->ulen) {
          ap->uname[ap->uname_read++] = ev->data[0];
          ap->uname[ap->uname_read] = 0;
        }
        if (ap->uname_read == ap->ulen) {
          parser_set_state(ap->p, AUTH_STATE_PLEN);
        }
        break;

      case AUTH_EVENT_PLEN:
        ap->plen = ev->data[0];
        ap->passwd_read = 0;
        ap->passwd[0] = 0;
        break;

      case AUTH_EVENT_PASSWD:
        if (ap->passwd_read < ap->plen) {
          ap->passwd[ap->passwd_read++] = ev->data[0];
          ap->passwd[ap->passwd_read] = 0;
        }
        if (ap->passwd_read == ap->plen) {
          access_level_t access_level;
          if (user_login(ap->uname, ap->passwd, &access_level) == USERS_OK) {
            ap->auth_status = AUTH_OK;
          } else {
            ap->auth_status = AUTH_ERROR;
          }
          parser_set_state(ap->p, AUTH_STATE_FIN);
          return AUTH_OK;
        }
        break;

      default:
        return AUTH_ERROR;
      }
    }
  }
}

auth_status_t auth_write(struct selector_key *key) {
  struct auth_parser *ap = ATTACHMENT(key);

  uint8_t response[2];
  response[0] = 0x01;
  response[1] = ap->auth_status;

  ssize_t n = send(key->fd, response, sizeof(response), 0);
  if (n != sizeof(response)) {
    // cliente cerró la conexión. Hay que manejar esto
    selector_unregister_fd(key->s, key->fd);
    return AUTH_ERROR;
  }
  return AUTH_OK;
}