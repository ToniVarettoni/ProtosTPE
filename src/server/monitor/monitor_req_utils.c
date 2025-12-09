#include "../include/monitor_req_utils.h"

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
    break;
  case REMOVE_USER:
    if (user_delete(mrq->uname) != USERS_OK) {
      return MONITOR_REQ_STATUS_ERR;
    }
    break;
  case CHANGE_PASSWD:
    if (user_change_password(mrq->uname, mrq->passwd) != USERS_OK) {
      return MONITOR_REQ_STATUS_ERR;
    }
    break;
  case STATISTICS:
    memcpy(&mrq->stats, get_stats(), sizeof(stats));
    break;
  default:
    return MONITOR_REQ_STATUS_ERR;
  }
  return MONITOR_REQ_STATUS_OK;
}
