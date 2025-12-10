#include "../include/monitor_req_utils.h"
#include "../../lib/logger/logger.h"

monitor_req_status_t handle_request(monitor_req_parser_t *mrq) {
  if (mrq == NULL) {
    return MONITOR_REQ_STATUS_ERR;
  }
  switch (mrq->type) {
  case ADD_USER:
    if (valid_user(mrq->uname, mrq->passwd) != USERS_OK &&
        user_create(mrq->uname, mrq->passwd, mrq->access_level) != USERS_OK) {
      return MONITOR_REQ_STATUS_ERR;
    }
    log_to_stdout("Created user with username %s and password %s\n", mrq->uname,
                  mrq->passwd);
    break;
  case REMOVE_USER:
    if (user_delete(mrq->uname) != USERS_OK) {
      return MONITOR_REQ_STATUS_ERR;
    }
    log_to_stdout("Deleted user %s\n", mrq->uname);
    break;
  case CHANGE_PASSWD:
    if (user_change_password(mrq->uname, mrq->passwd) != USERS_OK) {
      return MONITOR_REQ_STATUS_ERR;
    }
    log_to_stdout("Changed password for user %s\n", mrq->uname);
    break;
  case STATISTICS:
    memcpy(&mrq->stats, get_stats(), sizeof(stats));
    log_to_stdout("historic: %d\ncurrent: %d\ntransferred bytes: %d\n",
                  mrq->stats.historic_connections,
                  mrq->stats.current_connections, mrq->stats.transferred_bytes);
    break;
  default:
    return MONITOR_REQ_STATUS_ERR;
  }
  return MONITOR_REQ_STATUS_OK;
}
