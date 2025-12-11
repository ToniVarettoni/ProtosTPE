#include "../include/monitor_req_utils.h"
#include "../../lib/logger/logger.h"
#include "../include/active_monitor.h"
#include "auth_utils.h"

monitor_req_status_t handle_request(struct selector_key *key) {
  monitor_req_parser_t *mrq =
      &((monitor_t *)ATTACHMENT(key))->parser.req_parser;

  if (mrq == NULL) {
    return MONITOR_REQ_STATUS_ERR;
  }
  char *uname, *passwd, *access_level;
  switch (mrq->type) {
  case ADD_USER:
    uname = (char *)mrq->arguments[0];
    passwd = (char *)mrq->arguments[1];
    access_level = (char *)mrq->arguments[2];
    if (valid_user(uname, passwd) != USERS_OK &&
        user_create(uname, passwd, *access_level) != USERS_OK) {
      return MONITOR_REQ_STATUS_ERR;
    }
    log_to_stdout("Created user with username %s and password %s\n", uname,
                  passwd);
    break;
  case REMOVE_USER:
    uname = (char *)mrq->arguments[0];
    if (user_delete(uname) != USERS_OK) {
      return MONITOR_REQ_STATUS_ERR;
    }
    log_to_stdout("Deleted user %s\n", uname);
    break;
  case CHANGE_PASSWD:
    uname = (char *)mrq->arguments[0];
    passwd = (char *)mrq->arguments[1];
    if (user_change_password(uname, passwd) != USERS_OK) {
      return MONITOR_REQ_STATUS_ERR;
    }
    log_to_stdout("Changed password for user %s\n", uname);
    break;
  case STATISTICS:
    memcpy(&mrq->stats, get_stats(), sizeof(stats_t));
    break;
  case CHANGE_AUTH_METHODS:
    if (change_auth_methods(mrq->arguments) != AUTH_OK) {
      return MONITOR_REQ_STATUS_ERR;
    }
    break;
  default:
    return MONITOR_REQ_STATUS_ERR;
  }
  selector_set_interest_key(key, OP_WRITE);
  return MONITOR_REQ_STATUS_OK;
}

#define MAX_REPLY_SIZE 1 + 24

static uint64_t htonll(uint64_t x) {
  uint32_t high = htonl((uint32_t)(x >> 32));
  uint32_t low = htonl((uint32_t)(x & 0xFFFFFFFFULL));
  return ((uint64_t)high << 32) | low;
}

static void write_u64(uint8_t *buf, uint64_t value) {
  uint64_t n = htonll(value);
  memcpy(buf, &n, sizeof(uint64_t));
}

monitor_req_status_t send_response(struct selector_key *key) {
  monitor_req_parser_t *mrq =
      &((monitor_t *)ATTACHMENT(key))->parser.req_parser;

  uint8_t reply[MAX_REPLY_SIZE] = {0};
  size_t reply_size = 1;

  reply[0] = (uint8_t)mrq->req_status;

  if (mrq->type == STATISTICS) {
    uint64_t total_connections = mrq->stats.historic_connections;
    uint64_t bytes_transferred = mrq->stats.transferred_bytes;
    uint64_t current_connections = mrq->stats.current_connections;

    write_u64(&reply[1], total_connections);
    write_u64(&reply[9], bytes_transferred);
    write_u64(&reply[17], current_connections);

    reply_size = 1 + 24;
  }

  log_to_stdout("Sending %d bytes\n", reply_size);
  ssize_t n = send(key->fd, reply, reply_size, 0);
  if (n != reply_size) {
    return MONITOR_REQ_STATUS_ERR;
  }

  selector_set_interest_key(key, OP_NOOP);
  return MONITOR_REQ_STATUS_OK;
}
