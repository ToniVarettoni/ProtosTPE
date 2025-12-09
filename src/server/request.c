#define _POSIX_C_SOURCE 200809L

#include "request.h"
#include "include/client_utils.h"
#include "../lib/logger/logger.h"
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

#define SOCKS_REPLY_SUCCESS 0x00
#define SOCKS_REPLY_GENERAL_FAILURE 0x01
#define SOCKS_REPLY_NETWORK_UNREACHABLE 0x03
#define SOCKS_REPLY_HOST_UNREACHABLE 0x04
#define SOCKS_REPLY_CMD_NOT_SUPPORTED 0x07
#define SOCKS_REPLY_ATYP_NOT_SUPPORTED 0X08

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
    {ANY, REQUEST_STATE_FIN, act_fin_req, NULL}};

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

static unsigned handle_request_fin(struct selector_key *key,
                                   request_parser_t *rp) {
  client_t *client = ATTACHMENT(key);
  log_to_stdout("Request parsed! Remember, the address type is %x\n", rp->atyp);

  switch (rp->atyp) {

  case ATTYP_DOMAINNAME:
    log_to_stdout("Started domain name resolution...\n");
    selector_set_interest_key(key, OP_WRITE);
    return setup_lookup(key, (char *)rp->addr, rp->addr_len, rp->port);
  
  case ATTYP_IPV4:
    log_to_stdout("Started IPv4 connection...\n");
    client->dest_addr = calloc(1, sizeof(addrinfo));
    rp->sockAddress = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_addr = rp->ipv4,
        .sin_port = htons(rp->port),
    };
    *client->dest_addr = (struct addrinfo){
        .ai_family = AF_INET,
        .ai_addr = (struct sockaddr *)&rp->sockAddress,
        .ai_addrlen = sizeof(struct sockaddr_in),
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP
      };
    selector_set_interest_key(key, OP_WRITE);
    return DEST_CONNECT;
  
  case ATTYP_IPV6:
    log_to_stdout("Started IPv6 connection...\n");
    client->dest_addr = calloc(1, sizeof(addrinfo));
    rp->sockAddress6 = (struct sockaddr_in6){
        .sin6_family = AF_INET6,
        .sin6_addr = rp->ipv6,
        .sin6_port = htons(rp->port),
    };
    *client->dest_addr = (struct addrinfo){
        .ai_family = AF_INET6,
        .ai_addr = (struct sockaddr *)&rp->sockAddress6,
        .ai_addrlen = sizeof(struct sockaddr_in6),
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
    };
    selector_set_interest_key(key, OP_WRITE);
    return DEST_CONNECT;
  
  default:
    log_to_stdout("Error invalid addres type: %d\n", rp->atyp);
    selector_set_interest_key(key, OP_NOOP);
    return ERROR;
  }
}

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
  client->parser.request_parser.addr_read = 0;
  client->parser.request_parser.port_read = 0;
}

unsigned request_read(struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  request_parser_t *rp = &client->parser.request_parser;
  uint8_t buffer[1024];
  size_t n;

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
        return handle_request_fin(key, rp);
                
      case REQUEST_STATE_ERROR:
        log_to_stdout("Error parsing request\n");
        selector_set_interest_key(key, OP_NOOP);
        return ERROR;
      
      case REQUEST_STATE_CMD:
        rp->cmd = e->data[0];
        if (rp->cmd != CMD_CONNECT){
          log_to_stdout("CMD %d not supported\n", rp->cmd);
          selector_set_interest_key(key, OP_WRITE);
          rp->reply_status = SOCKS_REPLY_CMD_NOT_SUPPORTED;
          return REQUEST_WRITE;
        }
        log_to_stdout("Correctly parsed cmd: %d\n", rp->cmd);
        break;
      
      case REQUEST_STATE_ATYP:
        if(e->data[0] != SOCKS_ATYP_IPV4 || e->data[0] != SOCKS_ATYP_IPV6 || e->data[0] != SOCKS_ATYP_DOMAINNAME) {
          selector_set_interest_key(key, OP_WRITE);
          rp->reply_status = SOCKS_REPLY_ATYP_NOT_SUPPORTED;
          return REQUEST_WRITE;
        }
        rp->atyp = e->data[0];
        log_to_stdout("Address type: %x\n", rp->atyp);
        break;

      case REQUEST_STATE_DSTADDR_IPV4:
        if (rp->addr_read < IPV4_LEN) {
          rp->addr[rp->addr_read++] = e->data[0];
        }
        if (rp->addr_read == IPV4_LEN) {
          memcpy(&rp->ipv4, rp->addr, IPV4_LEN);
          rp->addr[rp->addr_read] = 0;
          log_to_stdout("Correctly parsed ipv4: %s\n", (char *)rp->addr);
          parser_set_state(rp->p, REQUEST_STATE_DSTPORT);
        }
        break;

      case REQUEST_STATE_DSTADDR_IPV6:
        if (rp->addr_read < IPV6_LEN) {
          rp->addr[rp->addr_read++] = e->data[0];
        }
        if (rp->addr_read == IPV6_LEN) {
          memcpy(&rp->ipv6, rp->addr, IPV6_LEN);
          parser_set_state(rp->p, REQUEST_STATE_DSTPORT);
        }
        break;

      case REQUEST_STATE_DSTADDR_DOMAINNAME:
        if (rp->addr_len == 0) {
          rp->addr_len = e->data[0];
          log_to_stdout("Address length is %d\n", rp->addr_len);
          break;
        }
        if (rp->addr_read < rp->addr_len) {
          rp->addr[rp->addr_read++] = e->data[0];
        }
        if (rp->addr_len == rp->addr_read) {
          rp->addr[rp->addr_read] = '\0';
          log_to_stdout("Domain name read is %s\n", rp->addr);
          parser_set_state(rp->p, REQUEST_STATE_DSTPORT);
        }
        break;

      case REQUEST_STATE_DSTPORT:
        if (rp->port_read == 0) {
          rp->port = ((uint16_t)e->data[0] << 8);
          rp->port_read = 1;
          log_to_stdout("Destination port is %d\n", rp->port);
        } else {
          rp->port |= e->data[0];
          rp->port_read = 2;
          log_to_stdout("Second read of destination port is %d\n", rp->port);
          parser_set_state(rp->p, REQUEST_STATE_FIN);
          const struct parser_event *fin_ev = parser_feed(rp->p, 0);
          for (; fin_ev != NULL; fin_ev = fin_ev->next) {
            if (fin_ev->type == REQUEST_STATE_FIN) {
              return handle_request_fin(key, rp);
            }
            if (fin_ev->type == REQUEST_STATE_ERROR) {
              selector_set_interest_key(key, OP_NOOP);
              return ERROR;
            }
          }
        }
        break;

      case REQUEST_STATE_VER:
        if (e->data[0] != 0x05){
          selector_set_interest_key(key, OP_WRITE);
          rp->reply_status = SOCKS_REPLY_GENERAL_FAILURE;
          return REQUEST_WRITE;
        }
        log_to_stdout("Protocol version (request): %d\n", e->data[0]);
        break;
      
      case REQUEST_STATE_RSV:
        log_to_stdout("Passing reserved byte...\n");
        break;
        
      
      default:
        log_to_stdout("Error: invalid request state: %d\n", e->type);
        selector_set_interest_key(key, OP_NOOP);
        return ERROR;
      }
    }
  }
  return REQUEST_READ;
}

unsigned setup_lookup(struct selector_key *key, char *addrname, uint8_t addrlen, uint16_t port) {
  client_t *client = ATTACHMENT(key);
  request_parser_t *rp = &client->parser.request_parser;
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

  int status = getaddrinfo_a(GAI_NOWAIT, reqs, 1, NULL);

  if (status != 0) {
    free((void *)req->ar_name);
    free(service);
    free(hints);
    free(req);
    selector_set_interest_key(key, OP_WRITE);
    rp->reply_status = SOCKS_REPLY_HOST_UNREACHABLE;
    return REQUEST_WRITE;
  }

  log_to_stdout("Passed the lookup setup!\n");

  return DNS_LOOKUP;
}

unsigned dns_lookup(struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  request_parser_t *rp = &client->parser.request_parser;
  if (client->dns_req == NULL) {
    selector_set_interest_key(key, OP_NOOP);
    return ERROR;
  }

  const struct gaicb *reqs[] = {(const struct gaicb *)client->dns_req};
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000}; // small wait to let lookup progress
  gai_suspend(reqs, 1, &ts);

  int status = gai_error(client->dns_req);
  if (status == EAI_INPROGRESS) {
    log_to_stdout("Lookup still going...\n");
    selector_set_interest_key(key, OP_WRITE);
    return DNS_LOOKUP;
  }

  if (status != 0) {
    if (status == EAI_NONAME || status == EAI_FAIL) {
      rp->reply_status = SOCKS_REPLY_NETWORK_UNREACHABLE;
    } else if (status == EAI_AGAIN || status == EAI_SYSTEM) {
      rp->reply_status = SOCKS_REPLY_HOST_UNREACHABLE;
    } else {
      rp->reply_status = SOCKS_REPLY_GENERAL_FAILURE;
    }
    log_to_stdout("Failed lookup.\n");
    selector_set_interest_key(key, OP_NOOP);
    free((void *)client->dns_req->ar_request);
    free((void *)client->dns_req->ar_service);
    free((void *)client->dns_req->ar_name);
    free(client->dns_req);
    client->dns_req = NULL;
    selector_set_interest_key(key, OP_WRITE);
    return REQUEST_WRITE;
  }

  client->dest_addr = client->dns_req->ar_result;

  char host[NI_MAXHOST];
  if (client->dest_addr != NULL &&
      getnameinfo(client->dest_addr->ai_addr, client->dest_addr->ai_addrlen,
                  host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0) {
    log_to_stdout("Lookup successful! Hostname is %s\n", host);
  } else {
    log_to_stdout("Lookup successful, using resolved address.\n");
  }

  free((void *)client->dns_req->ar_request);
  free((void *)client->dns_req->ar_service);
  free((void *)client->dns_req->ar_name);
  free(client->dns_req);
  client->dns_req = NULL;

  

  selector_set_interest_key(key, OP_WRITE);
  return DEST_CONNECT;
}

unsigned try_connect(struct selector_key *key) {
  client_t *client = ATTACHMENT(key);
  request_parser_t *rp = &client->parser.request_parser;
  struct addrinfo *addr = client->dest_addr;
  // me fijo si ya inicie una conexion
  if (client->destination_fd != -1) {
    int so_error = 0;
    socklen_t len = sizeof(so_error);
    if (getsockopt(client->destination_fd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
      log_to_stdout("Error: getsockopt failed for client %d: %s\n", client->client_fd, strerror(errno));
      selector_set_interest_key(key, OP_NOOP);
      return ERROR;
    }

    // la conexion en proceso termino
    if (so_error == 0) {
      selector_set_interest(key->s, client->client_fd, OP_WRITE);
      rp->reply_status = SOCKS_REPLY_SUCCESS;
      return REQUEST_WRITE;
    }

    // la conexion sigue en procesos
    if (so_error == EINPROGRESS || so_error == EALREADY) {
      return DEST_CONNECT;
    }

    // esta conexion fallo, asi que intento de conectarme al siguiente addr
    log_to_stdout("Error: failed to connect to remote for client: %d, errno: %s\n", client->client_fd, strerror(so_error));
    close(client->destination_fd);
    client->destination_fd = -1;
    if (addr != NULL) {
      addr = addr->ai_next;
      client->dest_addr = addr;
    }
  }

  // no hay conexion activa o la anterior no funciono
  for (; addr != NULL; addr = addr->ai_next) {
    int fd = socket(addr->ai_family, SOCK_STREAM, addr->ai_protocol);
    if (fd < 0) {
      continue;
    }

    selector_fd_set_nio(fd);
    errno = 0;
    int status = connect(fd, addr->ai_addr, addr->ai_addrlen);
    if (status == 0 || errno == EINPROGRESS) {
      client->destination_fd = fd;
      client->dest_addr = addr;
      if (selector_register(key->s, fd, get_client_handler(), OP_WRITE, key->data) != SELECTOR_SUCCESS) {
        close(fd);
        client->destination_fd = -1;
        continue;
      }

      if (status == 0) {
        selector_set_interest(key->s, client->client_fd, OP_WRITE);
        rp->reply_status = SOCKS_REPLY_SUCCESS;
        return REQUEST_WRITE;
      }

      return DEST_CONNECT;
    }

    log_to_stdout("connect() failed for client %d addr family %d: %s\n",
                  client->client_fd, addr->ai_family, strerror(errno));
    close(fd);
  }

  log_to_stdout("Error: failed to connect to remote for client: %d\n", client->client_fd);
  selector_set_interest_key(key, OP_NOOP);
  return ERROR;
}


unsigned request_write(struct selector_key *key) { 
  client_t *client = ATTACHMENT(key);
  uint8_t reply[4 + 16 + 2]; // VER REP RSV ATYP + ADDR + PORT
  size_t index = 0;

  reply[index++] = SOCKS_VERSION_5;
  reply[index++] = 0x00; // success (per request)
  reply[index++] = 0x00; // RSV

  struct sockaddr_storage socket_storage;
  socklen_t ss_len = sizeof(socket_storage);
  int family = AF_INET;
  memset(&socket_storage, 0, sizeof(socket_storage));

  // getsockname me retorna la direccion y el puerto usado por el proxy para llegar al remoto
  if (client->destination_fd != -1 &&
      getsockname(client->destination_fd, (struct sockaddr *)&socket_storage, &ss_len) == 0) { 
    family = ((struct sockaddr *)&socket_storage)->sa_family;
  } else {
    family = AF_INET;
  }

  // se carga el ATYP, la direccion y el puerto (va saltando usando sizeof)
  if (family == AF_INET) {
    struct sockaddr_in *sin4 = (struct sockaddr_in *)&socket_storage;
    reply[index++] = SOCKS_ATYP_IPV4;
    memcpy(&reply[index], &sin4->sin_addr, sizeof(sin4->sin_addr));
    index += sizeof(sin4->sin_addr);
    memcpy(&reply[index], &sin4->sin_port, sizeof(sin4->sin_port));
    index += sizeof(sin4->sin_port);
  // idem pero IPv6
  } else if (family == AF_INET6) {
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&socket_storage;
    reply[index++] = SOCKS_ATYP_IPV6;
    memcpy(&reply[index], &sin6->sin6_addr, sizeof(sin6->sin6_addr));
    index += sizeof(sin6->sin6_addr);
    memcpy(&reply[index], &sin6->sin6_port, sizeof(sin6->sin6_port)); // network order
    index += sizeof(sin6->sin6_port);
  }

  ssize_t n = send(key->fd, reply, index, 0);
  if (n != (ssize_t)index) {
    selector_set_interest_key(key, OP_NOOP);
    return ERROR;
  }

  selector_set_interest(key->s, client->client_fd, OP_READ);
  if (client->destination_fd != -1) {
    selector_set_interest(key->s, client->destination_fd, OP_READ);
  }
  return FORWARDING; 
}
