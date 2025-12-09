#ifndef _PASSIVE_MONITOR_H_
#define _PASSIVE_MONITOR_H_

#include "../../lib/stm/stm.h"
#include "monitor_auth.h"
#include "monitor_req.h"
#include "monitor_res.h"
#include "users.h"

const fd_handler *get_passive_monitor_handler();

const fd_interest get_passive_monitor_interests();

#endif