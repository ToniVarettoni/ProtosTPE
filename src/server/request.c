#include "request.h"
#include "include/client_utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void act_dstaddripv4_req(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_DSTADDR_IPV4;
  ret->data[0] = c;
  ret->n = 1;
}

static void act_dstaddripv6_req(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_DSTADDR_IPV6;
  ret->data[0] = c;
  ret->n = 1;
}

static void act_dstaddr_domainname_req(struct parser_event *ret, const uint8_t c) {
  ret->type = REQUEST_STATE_DSTADDR_DOMAINNAME;
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
    {SOCKS_ATYP_IPV4, REQUEST_STATE_DSTADDR_IPV4, act_atyp_req, NULL},
    {SOCKS_ATYP_IPV6, REQUEST_STATE_DSTADDR_IPV6, act_atyp_req, NULL},
    {SOCKS_ATYP_DOMAINNAME, REQUEST_STATE_DSTADDR_DOMAINNAME, act_atyp_req, NULL},
    {ANY, REQUEST_ATYP_ERROR, act_error_req, NULL}};

static struct parser_state_transition ST_DSTADDR_IPV4[] = {
    {ANY, REQUEST_STATE_DSTADDR_IPV4, act_dstaddripv4_req, NULL}};

static struct parser_state_transition ST_DSTADDR_IPV6[] = {
    {ANY, REQUEST_STATE_DSTADDR_IPV6, act_dstaddripv6_req, NULL}};

static struct parser_state_transition ST_DSTADDR_DOMAINNAME[] = {
    {ANY, REQUEST_STATE_DSTADDR_DOMAINNAME, act_dstaddr_domainname_req, NULL}};

static struct parser_state_transition ST_DSTPORT[] = {
    {ANY, REQUEST_STATE_DSTPORT, act_dstport_req, NULL}};

static struct parser_state_transition ST_FIN[] = {
    {ANY, REQUEST_STATE_ERROR, act_error_req, NULL}};

static struct parser_state_transition ST_ERROR[] = {
    {ANY, REQUEST_STATE_ERROR, act_error_req, NULL}};

static const struct parser_state_transition *states[] = {
    ST_VER, ST_CMD, ST_RSV, ST_ATYP, ST_DSTADDR_IPV4, ST_DSTADDR_IPV6, ST_DSTADDR_DOMAINNAME, ST_DSTPORT, ST_FIN, ST_ERROR,
};

static const size_t states_n[] = {
    sizeof(ST_VER) / sizeof(ST_VER[0]),
    sizeof(ST_CMD) / sizeof(ST_CMD[0]),
    sizeof(ST_RSV) / sizeof(ST_RSV[0]),
    sizeof(ST_ATYP) / sizeof(ST_ATYP[0]),
    sizeof(ST_DSTADDR_IPV4) / sizeof(ST_DSTADDR_IPV4[0]),
    sizeof(ST_DSTADDR_IPV6) / sizeof(ST_DSTADDR_IPV6[0]),
    sizeof(ST_DSTADDR_DOMAINNAME) / sizeof(ST_DSTADDR_DOMAINNAME[0]),
    sizeof(ST_DSTPORT) / sizeof(ST_DSTPORT[0]),
    sizeof(ST_FIN) / sizeof(ST_FIN[0]),
    sizeof(ST_ERROR) / sizeof(ST_ERROR[0]),
};

static const struct parser_definition request_parser_def = {
    .states_count = 10,
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
  size_t n, addr_count = 0, port_count = 0;

  n = recv(key->fd, buffer, sizeof(buffer), 0);

  if (n <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return REQUEST_READ;
    }
    log_to_stdout("Error parsing the request.\n");
    selector_set_interest_key(key, OP_NOOP);
    return ERROR;
  }

  for (size_t i = 0; i < n; i++) {
    const struct parser_event *e = parser_feed(rp->p, buffer[i]);
    if (e != NULL) {
      switch (e->type){
      case REQUEST_STATE_FIN:
        printf("Request parsed!\n");
        switch (rp->atyp){
          case ATTYP_DOMAINNAME:
            selector_set_interest_key(key, OP_NOOP);          
            return setup_lookup(key, rp->addr, rp->addr_len, rp->port);

          case ATTYP_IPV4:
            client->dest_addr = calloc(1, sizeof(addrinfo));
            rp->sockAddress = (struct sockaddr_in) {
              .sin_family = AF_INET,
              .sin_addr = rp->ipv4,
              .sin_port = htons(rp->port),
            };
            *client->dest_addr = (struct addrinfo) {
              .ai_family = AF_INET,
              .ai_addr = (struct sockaddr *) &rp->sockAddress,
              .ai_addrlen = sizeof(struct sockaddr_in),
              .ai_socktype = SOCK_STREAM,
              .ai_protocol = IPPROTO_TCP
            };  
            selector_set_interest_key(key, OP_NOOP);
            return DEST_CONNECT;

          case ATTYP_IPV6:
            client->dest_addr = calloc(1, sizeof(addrinfo));
            rp->sockAddress6 = (struct sockaddr_in6) {
                  .sin6_family = AF_INET6,
                  .sin6_addr = rp->ipv6,
                  .sin6_port = htons(rp->port),
              };
            *client->dest_addr = (struct addrinfo) {
                  .ai_family = AF_INET6,
                  .ai_addr = (struct sockaddr *) &rp->sockAddress6,
                  .ai_addrlen = sizeof(struct sockaddr_in6),
            };  
            selector_set_interest_key(key, OP_NOOP);
            return DEST_CONNECT;

          default:
            printf("Error invalid addres type: %d\n", rp->atyp);
            selector_set_interest_key(key, OP_NOOP);
            return ERROR;
        }
                
      case REQUEST_STATE_ERROR:
        printf("Error parsing request\n");
        selector_set_interest_key(key, OP_NOOP);
        return ERROR;
      
      case REQUEST_STATE_CMD:
        rp->cmd = e->data[0];
        if (rp->cmd != CMD_BIND && rp->cmd != CMD_CONNECT && rp->cmd != CMD_UDP_ASSOCIATE){
          log_to_stdout("Error parsing request cmd: %d\n", rp->cmd);
          selector_set_interest_key(key, OP_NOOP);
          return ERROR;
        }
        printf("correctly parsed cmd: %d", rp->cmd);
        break;
      
      case REQUEST_STATE_ATYP:
        rp->atyp = e->data[0];
        break;

      case REQUEST_STATE_DSTADDR_IPV4:
        if (rp->addr_len < IPV4_LEN) {
          rp->addr[rp->addr_len++] = e->data[0];
        }
        if (rp->addr_len == IPV4_LEN) {
          memcpy(&rp->ipv4, rp->addr, IPV4_LEN);
          rp->addr[rp->addr_len] = 0;
          printf("correctly parsed ipv4: %s", rp->addr);
          parser_set_state(rp->p, REQUEST_STATE_DSTPORT);
        }
        break;

      case REQUEST_STATE_DSTADDR_IPV6:
        if (rp->addr_len < IPV6_LEN) {
          rp->addr[rp->addr_len++] = e->data[0];
        }
        if (rp->addr_len == IPV6_LEN) {
          memcpy(&rp->ipv6, rp->addr, IPV6_LEN);
          parser_set_state(rp->p, REQUEST_STATE_DSTPORT);
        }
        break;

      case REQUEST_STATE_DSTADDR_DOMAINNAME:
        if (rp->addr_len == 0) {
          rp->addr_len = e->data[0];
          break;
        }
        if (addr_count < rp->addr_len) {
          rp->addr[addr_count++] = e->data[0];
        }
        if (rp->addr_len == addr_count) {
          rp->addr[addr_count] = 0;
          parser_set_state(rp->p, REQUEST_STATE_DSTPORT);
        }
        break;

      case REQUEST_STATE_DSTPORT:
        if (port_count == 0) {
          rp->port = ((uint16_t)e->data[0] << 8);
          port_count++;
        } else {
          rp->port |= e->data[0];
          parser_set_state(rp->p, REQUEST_STATE_FIN);
        }
        break;

      case REQUEST_STATE_VER:
        if (e->data[0] != 0x05){
          log_to_stdout("Version %d is not valid or something else went wrong.\n");
          selector_set_interest_key(key, OP_NOOP);
          return ERROR;
        }
        log_to_stdout("Protocol version (request): %d\n", e->data[0]);
        break;
      case REQUEST_STATE_RSV:
        break;
        
      
      default:
        printf("Error: invalid request state: %d\n", e->type);
        selector_set_interest_key(key, OP_NOOP);
        return ERROR;
      }
    }
  }
  return REQUEST_READ;
}

unsigned setup_lookup(struct selector_key *key, char ** addrname, uint8_t addrlen, uint16_t port) {

  client_t *client = ATTACHMENT(key);
  struct gaicb *req = calloc(1, sizeof(struct gaicb));
  client->dns_req = req;

  char *service = malloc(6);
  snprintf(service, 6, "%u", port);

  req->ar_name = strdup(addrname);
  req->ar_service = service;

  struct addrinfo *hints = calloc(1, sizeof(struct addrinfo));
  hints->ai_family = AF_UNSPEC;
  hints->ai_socktype = SOCK_STREAM;

  req->ar_request = hints;

  struct gaicb *reqs[1] = { req };

  req->ar_name = strdup(addrname);

  int rc = getaddrinfo_a(GAI_NOWAIT, reqs, 1, NULL);

  if (rc != 0) {
    return ERROR;
  }

  return DNS_LOOKUP;
}

void try_connect(const unsigned state, struct selector_key *key) {
  client_t * client = ATTACHMENT(key);
  selector_set_interest_key(key, OP_WRITE);
  struct addrinfo* addr = client->dest_addr;
  while (addr != NULL && client->destination_fd != -1){
    client->destination_fd = socket(addr->ai_family, SOCK_STREAM, addr->ai_protocol);
    if (client->destination_fd < 0){
      printf("Error: failed to create remote socket for client: %d", client->client_fd);
      return ERROR;
    }

    selector_fd_set_nio(client->destination_fd);
    errno = 0;
    if (connect(client->destination_fd, addr->ai_addr, addr->ai_addrlen) == 0 || errno == EINPROGRESS){
      selector_register(key->s, client->destination_fd, get_client_handler, OP_WRITE, key->data);
      return DEST_CONNECT;
    }else{
      printf("Error: failed to connect  to remote for client: %d, errno: %s\n", client->client_fd, strerror(errno));
      close(client->destination_fd);
      client->destination_fd = -1;
      addr = addr->ai_next;
    }
  }
  printf("Error: failed to connect  to remote for client: %d\n", client->client_fd, strerror(errno));
  return ERROR;
}


unsigned request_write(struct selector_key *key) { return 0; }
