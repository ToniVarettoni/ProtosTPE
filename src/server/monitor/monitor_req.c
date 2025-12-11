#include "../include/monitor_req.h"
#include "../../lib/logger/logger.h"
#include "../../lib/parser/parser.h"
#include "../include/active_monitor.h"
#include "../include/client_utils.h"

#include "../include/monitor_req_utils.h"

#define ADD_USER_ARGS_AMOUNT 3
#define REMOVE_USER_ARGS_AMOUNT 1
#define CHANGE_PASSWD_ARGS_AMOUNT 2
#define STATISTICS_REQ_ARGS_AMOUNT 0

// this array is ordeder by the enum values of the request types (ex: ADD_USER
// == 0, set to position 0)
static const uint8_t max_arguments[REQ_AMOUNT] = {
    ADD_USER_ARGS_AMOUNT, REMOVE_USER_ARGS_AMOUNT, CHANGE_PASSWD_ARGS_AMOUNT,
    STATISTICS_REQ_ARGS_AMOUNT};

static void act_monitor_req_type(struct parser_event *ret, const uint8_t c) {
  ret->type = MONITOR_REQ_EVENT_TYPE;
  ret->n = 1;
  ret->data[0] = c;
}

static void act_monitor_req_argument_length(struct parser_event *ret,
                                            const uint8_t c) {
  ret->type = MONITOR_REQ_EVENT_ARGUMENT_LENGTH;
  ret->n = 1;
  ret->data[0] = c;
}
static void act_monitor_req_argument(struct parser_event *ret,
                                     const uint8_t c) {
  ret->type = MONITOR_REQ_EVENT_ARGUMENT;
  ret->n = 1;
  ret->data[0] = c;
}
static void act_monitor_req_done(struct parser_event *ret, const uint8_t c) {
  ret->type = MONITOR_REQ_EVENT_DONE;
  ret->n = 0;
}
static void act_monitor_req_error(struct parser_event *ret, const uint8_t c) {
  ret->type = MONITOR_REQ_EVENT_ERROR;
  ret->n = 0;
}

static struct parser_state_transition ST_TYPE[] = {
    {ANY, MONITOR_REQ_STATE_ARGUMENT_LENGTH, act_monitor_req_type, NULL},
};

static struct parser_state_transition ST_ARGUMENT_LENGTH[] = {
    {ANY, MONITOR_REQ_STATE_ARGUMENT, act_monitor_req_argument_length, NULL},
};

static struct parser_state_transition ST_ARGUMENT[] = {
    {ANY, MONITOR_REQ_STATE_ARGUMENT, act_monitor_req_argument, NULL},
};

static struct parser_state_transition ST_DONE[] = {
    {ANY, MONITOR_REQ_STATE_DONE, act_monitor_req_done, NULL},
};

static struct parser_state_transition ST_ERROR[] = {
    {ANY, MONITOR_REQ_STATE_ERROR, act_monitor_req_error, NULL},
};

static const struct parser_state_transition *states[] = {
    ST_TYPE, ST_ARGUMENT_LENGTH, ST_ARGUMENT, ST_DONE, ST_ERROR};

static const size_t states_n[] = {sizeof(ST_TYPE) / sizeof(ST_TYPE[0]),
                                  sizeof(ST_ARGUMENT_LENGTH) /
                                      sizeof(ST_ARGUMENT_LENGTH[0]),
                                  sizeof(ST_ARGUMENT) / sizeof(ST_ARGUMENT[0]),
                                  sizeof(ST_DONE) / sizeof(ST_DONE[0]),
                                  sizeof(ST_ERROR) / sizeof(ST_ERROR[0])};

static const struct parser_definition monitor_req_parser_def = {
    .states_count = 5,
    .states = states,
    .states_n = states_n,
    .start_state = MONITOR_REQ_STATE_TYPE};

static monitor_req_status_t monitor_req_parser_init(monitor_req_parser_t *mrq) {
  memset(mrq, 0, sizeof(monitor_req_parser_t));
  mrq->p = parser_init(parser_no_classes(), &monitor_req_parser_def);
  if (mrq->p == NULL) {
    return MONITOR_REQ_STATUS_ERR;
  }
  return MONITOR_REQ_STATUS_OK;
}

void monitor_req_init(const unsigned state, struct selector_key *key) {
  monitor_t *monitor = ATTACHMENT(key);
  if (monitor_req_parser_init(&monitor->parser.req_parser) !=
      MONITOR_REQ_STATUS_OK) {
    error_handler(MONITOR_REQ, key);
  }
  monitor->active_parser = REQ_PARSER;
}

void monitor_req_finalize(const unsigned state, struct selector_key *key) {
  monitor_t *monitor = ATTACHMENT(key);
  parser_destroy(monitor->parser.req_parser.p);
}

unsigned monitor_req_read(struct selector_key *key) {
  monitor_t *monitor = ATTACHMENT(key);
  monitor_req_parser_t *mrq = &monitor->parser.req_parser;
  uint8_t c;
  ssize_t n = recv(key->fd, &c, 1, 0);
  if (n <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return MONITOR_REQ;
    }
    monitor->error = MONITOR_IO_ERROR;
    return MONITOR_ERROR;
  }

  const struct parser_event *ev = parser_feed(mrq->p, c);
  for (; ev != NULL; ev = ev->next) {
    switch (ev->type) {

    case MONITOR_REQ_EVENT_TYPE:
      mrq->type = ev->data[0];
      break;

      case MONITOR_REQ_EVENT_ARGUMENT_LENGTH:
        if (ev->data[0] == 0) {
          if (mrq->arguments_read != max_arguments[mrq->type]) {
            return MONITOR_ERROR;
          }
          mrq->req_status = handle_request(key);
          if (mrq->req_status != MONITOR_REQ_STATUS_OK) {
            return MONITOR_ERROR;
          }
          return MONITOR_REQ;
        }
        mrq->current_argument_length = ev->data[0];
        mrq->current_argument_read = 0;
        mrq->arguments_read++;
        break;
      case MONITOR_REQ_EVENT_ARGUMENT:
      if (mrq->current_argument_read < mrq->current_argument_length) {
        switch (mrq->arguments_read) {
        case 1:
          mrq->uname[mrq->current_argument_read++] = ev->data[0];
          mrq->uname[mrq->current_argument_read] = '\0';
          break;
        case 2:
          mrq->passwd[mrq->current_argument_read++] = ev->data[0];
          mrq->passwd[mrq->current_argument_read] = '\0';
          break;
        case 3:
          mrq->access_level = ev->data[0];
          mrq->current_argument_read++;
          break;
        default:
          monitor->error = MONITOR_UNKNOWN_ERROR;
          return MONITOR_ERROR;
        }
      }
      if (mrq->current_argument_read == mrq->current_argument_length) {
        parser_set_state(mrq->p, MONITOR_REQ_STATE_ARGUMENT_LENGTH);
      }
      break;
    case MONITOR_REQ_EVENT_DONE:
      return MONITOR_REQ;
    case MONITOR_REQ_EVENT_ERROR:
      monitor->error = MONITOR_UNKNOWN_ERROR;
      return MONITOR_ERROR;
    default:
      monitor->error = MONITOR_UNKNOWN_ERROR;
      return MONITOR_ERROR;
    }
  }
  return MONITOR_REQ;
}

unsigned monitor_req_write(struct selector_key *key) {
  monitor_t *monitor = ATTACHMENT(key);
  if (send_response(key) != MONITOR_REQ_STATUS_OK) {
    log_to_stdout("Failed to send response to monitor client\n");
    monitor->error = MONITOR_IO_ERROR;
    return MONITOR_ERROR;
  }
  return MONITOR_DONE;
}
