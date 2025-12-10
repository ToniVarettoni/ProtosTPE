#include "include/monitor_utils.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>

#define ACCESS_LEVEL_ARG_LENGTH 0x01

#define ADD_USER_ACTION_TYPE 0x00
#define DELETE_USER_ACTION_TYPE 0x01
#define CHANGE_PASS_ACTION_TYPE 0x02

#define TERMINATOR 0x00

size_t write_monitor_auth_request(uint8_t *buffer, size_t buffer_len,
                                  const user_t *login_user) {
  if (buffer == NULL || login_user == NULL || login_user->username == NULL || login_user->password == NULL) {
    return 0;
  }

  size_t ulen = strlen(login_user->username);
  size_t plen = strlen(login_user->password);

  if (ulen == 0 || ulen > UINT8_MAX || plen == 0 || plen > UINT8_MAX) {
    return 0;
  }

  size_t needed = 2 + ulen + plen;
  if (needed > buffer_len) {
    return 0;
  }

  size_t pos = 0;
  buffer[pos++] = (uint8_t)ulen;
  memcpy(buffer + pos, login_user->username, ulen);
  pos += ulen;
  buffer[pos++] = (uint8_t)plen;
  memcpy(buffer + pos, login_user->password, plen);
  pos += plen;

  return pos;
}

bool read_monitor_auth_reply(int sockfd) {
  uint8_t status = 0xFF;
  ssize_t n = recv(sockfd, &status, 1, 0);
  if (n <= 0) {
    return false;
  }
  return status == 0x00;
}

size_t write_monitor_user_add_request(uint8_t *buffer, size_t buffer_len, const user_t *user_to_add) {
  if (buffer == NULL || user_to_add == NULL || user_to_add->username == NULL || user_to_add->password == NULL) {
    return 0;
  }

  size_t ulen = strlen(user_to_add->username);
  size_t plen = strlen(user_to_add->password);

  if (ulen == 0 || ulen > UINT8_MAX || plen == 0 || plen > UINT8_MAX) {
    return 0;
  }

  // type + len(uname) + uname + len(pass) + pass + len(access) + access + terminator
  size_t needed = 6 + ulen + plen;
  if (needed > buffer_len) {
    return 0;
  }

  size_t pos = 0;
  buffer[pos++] = ADD_USER_ACTION_TYPE;
  buffer[pos++] = (uint8_t)ulen;
  memcpy(buffer + pos, user_to_add->username, ulen);
  pos += ulen;
  buffer[pos++] = (uint8_t)plen;
  memcpy(buffer + pos, user_to_add->password, plen);
  pos += plen;
  buffer[pos++] = ACCESS_LEVEL_ARG_LENGTH;
  buffer[pos++] = user_to_add->access_level;
  buffer[pos++] = TERMINATOR; 

  return pos;

}

size_t write_monitor_user_delete_request(uint8_t *buffer, size_t buffer_len, const user_t *user_to_delete) {
  if (buffer == NULL || user_to_delete == NULL || user_to_delete->username == NULL) {
    return 0;
  }

  size_t ulen = strlen(user_to_delete->username);

  if(ulen == 0 || ulen > UINT8_MAX) {
    return 0;
  }

  // type + len(uname) + uname + terminator
  size_t needed = 3 + ulen;
  if (needed > buffer_len) {
    return 0;
  }

  size_t pos = 0;
  buffer[pos++] = DELETE_USER_ACTION_TYPE;
  buffer[pos++] = (uint8_t)ulen;
  memcpy(buffer + pos, user_to_delete->username, ulen);
  pos += ulen;
  buffer[pos++] = TERMINATOR;

  return pos;

}

size_t write_monitor_change_pass_request(uint8_t *buffer, size_t buffer_len,
                                         const user_t *user_to_change) {
  if (buffer == NULL || user_to_change == NULL || user_to_change->username == NULL ||
      user_to_change->password == NULL) {
    return 0;
  }

  size_t ulen = strlen(user_to_change->username);
  size_t plen = strlen(user_to_change->password);

  if (ulen == 0 || ulen > UINT8_MAX || plen == 0 || plen > UINT8_MAX) {
    return 0;
  }

  // type + len(uname) + uname + len(pass) + pass + terminator
  size_t needed = 5 + ulen + plen;
  if (needed > buffer_len) {
    return 0;
  }

  size_t pos = 0;
  buffer[pos++] = CHANGE_PASS_ACTION_TYPE;
  buffer[pos++] = (uint8_t)ulen;
  memcpy(buffer + pos, user_to_change->username, ulen);
  pos += ulen;
  buffer[pos++] = (uint8_t)plen;
  memcpy(buffer + pos, user_to_change->password, plen);
  pos += plen;
  buffer[pos++] = TERMINATOR;

  return pos;
}
