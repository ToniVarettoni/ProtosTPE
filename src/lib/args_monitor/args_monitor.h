#ifndef ARGS_MONITOR_H
#define ARGS_MONITOR_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_METHODS 255

typedef enum {
  ACTION_NONE = -1,
  ACTION_ADD_USER,
  ACTION_DELETE_USER,
  ACTION_CHANGE_PASSWORD,
  ACTION_STATS,
  ACTION_CHANGE_AUTH_METHODS
} management_action_t;

typedef struct {
  char *username;
  char *password;
  uint8_t access_level;
} user_t;

typedef struct {
  int8_t auth_methods[MAX_METHODS];
  uint8_t auth_methods_count;
  user_t managing_user;
  management_action_t action;
  user_t user_to_modify;
  char *direction;
  unsigned short port;
} management_args_t;

void parse_monitor_args(const int argc, char **argv, management_args_t *args);

#endif
