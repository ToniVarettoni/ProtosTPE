#include "request.h"
#include "include/client_utils.h"

#define SOCKS_VERSION_5 0x05

#define SOCKS_ATYP_IPV4 0x01
#define SOCKS_ATYP_DOMAINNAME 0x03
#define SOCKS_ATYP_IPV6 0x04

#define SOCKS_CMD_CONNECT 0x01
#define SOCKS_CMD_BIND 0x02
#define SOCKS_CMD_UDP_ASSOCIATE 0x03

static void act_ver_req(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_VER;
  ret->data[0] = c;
  ret->n = 1;
}

static void act_cmd_req(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_CMD;
  ret->data[0] = c;
  ret->n = 1;
}

static void act_rsv_req(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_RSV;
  ret->data[0] = c;
  ret->n = 1;
}

static void act_atyp_req(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_ATYP;
  ret->data[0] = c;
  ret->n = 1;
}

static void act_dstaddr_req(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_DSTADDR;
  ret->data[0] = c;
  ret->n = 1;
}

static void act_dstport_req(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_DSTPORT;
  ret->data[0] = c;
  ret->n = 1;
}

static void act_fin_req(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_FIN;
  ret->n = 0;
}

static void act_error_req(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_ERROR;
  ret->n = 0;
}

static struct parser_state_transition ST_VER[] = {
    {ANY, REQUEST_STATE_CMD, act_ver_req, NULL}};

static struct parser_state_transition ST_CMD[] = {
    {ANY, REQUEST_STATE_RSV, act_cmd_req, NULL}};

static struct parser_state_transition ST_RSV[] = {
    {ANY, REQUEST_STATE_ATYP, act_rsv_req, NULL}};

static struct parser_state_transition ST_ATYP[] = {
    {ANY, REQUEST_STATE_DSTADDR, act_atyp_req, NULL}};

static struct parser_state_transition ST_DSTADDR[] = {
    {ANY, REQUEST_STATE_DSTPORT, act_dstaddr_req, NULL}};

static struct parser_state_transition ST_DSTPORT[] = {
    {ANY, REQUEST_STATE_FIN, act_dstport_req, NULL}};

static struct parser_state_transition ST_FIN[] = {
    {ANY, REQUEST_STATE_ERROR, act_error_req, NULL}};

static struct parser_state_transition ST_ERROR[] = {
    {ANY, REQUEST_STATE_ERROR, act_error_req, NULL}};

static const struct parser_state_transition *states[] = {
    ST_VER, ST_CMD, ST_RSV, ST_ATYP, ST_DSTADDR, ST_DSTPORT, ST_FIN, ST_ERROR,
};

static const size_t states_n[] = {
    sizeof(ST_VER) / sizeof(ST_VER[0]),
    sizeof(ST_CMD) / sizeof(ST_CMD[0]),
    sizeof(ST_RSV) / sizeof(ST_RSV[0]),
    sizeof(ST_ATYP) / sizeof(ST_ATYP[0]),
    sizeof(ST_DSTADDR) / sizeof(ST_DSTADDR[0]),
    sizeof(ST_DSTPORT) / sizeof(ST_DSTPORT[0]),
    sizeof(ST_FIN) / sizeof(ST_FIN[0]),
    sizeof(ST_ERROR) / sizeof(ST_ERROR[0]),
};

static const struct parser_definition request_parser_def = {
    .states_count = 8,
    .states = states,
    .states_n = states_n,
    .start_state = REQUEST_STATE_VER,
};

request_status_t request_parser_init(request_parser_t *hp) {
  memset(hp, 0, sizeof(request_parser_t));
  hp->p = parser_init(parser_no_classes(), &request_parser_def);
  if (hp->p == NULL) {
    return REQUEST_UNKNOWN_ERROR;
  }
  return REQUEST_OK;
}

void request_read_init(const unsigned state, struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  request_status_t status = request_parser_init(&client->parser.request_parser);
  if (status != REQUEST_OK) {
    close_connection(key);
  }
}

unsigned request_read(struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  request_parser_t *rp = &client->parser.request_parser;
  uint8_t buffer[1024];
  size_t n;

  n = recv(key->fd, buffer, sizeof(buffer), 0);

  for (size_t i = 0; i < n; i++) {
    const struct parser_event *e = parser_feed(rp->p, buffer[i]);
    if (e != NULL) {
      if (e->type == REQUEST_STATE_FIN) {
        printf("Request parsed!\n");
        return REQUEST_STATE_FIN;
      }
      if (e->type == REQUEST_STATE_ERROR) {
        printf("Error parsing request\n");
        return REQUEST_STATE_ERROR;
      }
    }
  }

  return REQUEST_STATE_VER;
}

void dns_lookup(const unsigned state, struct selector_key *key) {}

void try_connect(const unsigned state, struct selector_key *key) {}

unsigned request_write(struct selector_key *key) { return 0; }