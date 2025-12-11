#ifndef MONITOR_UTILS_H
#define MONITOR_UTILS_H

#include "../../lib/args_monitor/args_monitor.h"
#include "../../server/include/stats.h"
#include <stddef.h>
#include <stdint.h>

// construye el request de autenticacion para enviar al servidor
size_t write_monitor_auth_request(uint8_t *buffer, size_t buffer_len, const user_t *login_user);

// lee el byte que le retorno el servidor. 0x00 para autenticacion exitosa y 
// 0x01 para autenticacion fallada
bool read_monitor_auth_reply(int sockfd);

size_t write_monitor_user_add_request(uint8_t *buffer, size_t buffer_len, const user_t *user_to_add);

size_t write_monitor_user_delete_request(uint8_t *buffer, size_t buffer_len, const user_t *user_to_delete);

size_t write_monitor_change_pass_request(uint8_t *buffer, size_t buffer_len, const user_t *user_to_change);

size_t write_monitor_get_stats_request(uint8_t *buffer, size_t buffer_len);

bool read_monitor_stats_reply(int sockfd, stats_t *out_stats);

#endif
