#ifndef _MONITOR_REQ_UTILS_H_
#define _MONITOR_REQ_UTILS_H_

#include "monitor_req.h"

monitor_req_status_t handle_request(struct selector_key *key);

monitor_req_status_t send_response(struct selector_key *key);

#endif