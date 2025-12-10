#include "../include/users.h"

#include "../lib/logger/logger.h"
#include <ctype.h>
#include <stdio.h>

#define READ_MODE "r"
#define WRTIE_MODE "w"

#define ADMIN_ACCESS_LEVEL_SPECIFIER '@'
#define USER_ACCESS_LEVEL_SPECIFIER '#'
#define USERNAME_DELIMITER ':'

#define MINIMUN_PRINTABLE_ASCII 32

static user_t *users;

static int users_size = 0;

static char *users_file_path;

static int get_user_index(char *username) {
  if (username == NULL) {
    return -1;
  }

  for (int i = 0; i < users_size; i++) {
    if (strcmp(users[i].username, username) == 0) {
      return i;
    }
  }
  return -1;
}

static int skipLine(FILE *file, unsigned int *line) {
  char c;
  do {
    c = fgetc(file);

    if (c == '\n')
      (*line)++;
  } while (c >= 0 && c != '\n');
  return c;
}

static int skipWhiteSpaces(FILE *file, unsigned int *line) {
  char c;
  do {
    c = fgetc(file);

    if (c == '\n')
      (*line)++;
  } while (isspace(c));
  return c;
}

static user_status parse_user(FILE *users_file, user_t *parsed_user,
                              unsigned int *line) {
  char c;
  c = skipWhiteSpaces(users_file, line);

  // If EOF found, end parsing
  if (c < 0)
    return -1;

  // First character will be access level specifier
  if (c == ADMIN_ACCESS_LEVEL_SPECIFIER) {
    parsed_user->access_level = ADMIN;
  } else if (c == USER_ACCESS_LEVEL_SPECIFIER) {
    parsed_user->access_level = USER;
  } else {
    return 1;
  }

  // Parse username
  int username_length = 0;
  while ((c = fgetc(users_file)) >= 0 && c != USERNAME_DELIMITER) {

    if (c < 32 || username_length > MAX_USERNAME_LEN) {
      // invalid char at line :line
      if (c != '\n')
        return skipLine(users_file, line);
      (*line)++;
      return 1;
    }
    parsed_user->username[username_length++] = c;
  }
  parsed_user->username[username_length] = '\0';

  // Parse password until newline/EOF
  int password_length = 0;
  while ((c = fgetc(users_file)) >= 0 && c != '\n') {
    if (c < 32 || password_length > MAX_PASSWORD_LEN) {
      // invalid char at line :line
      return skipLine(users_file, line);
    }
    parsed_user->password[password_length++] = c;
  }
  parsed_user->password[password_length] = '\0';
  if (c == '\n') {
    (*line)++;
  }
  return 0;
}

static user_status load_users(char *users_file_path) {

  FILE *file = fopen(users_file_path, READ_MODE);

  if (file == NULL) {
    // File not found. Not necessarily an error.
    return -1;
  }

  unsigned int line = 1;
  user_t parsed_user;

  int result;
  do {
    result = parse_user(file, &parsed_user, &line);
    if (result != 0)
      continue;

    user_status status = user_create(parsed_user.username, parsed_user.password,
                                     parsed_user.access_level);
    switch (status) {
    case USERS_INVALID_USERNAME:
      log_to_stdout("invalid username at line %u\n", line);
      break;
    case USERS_INVALID_PASSWORD:
      log_to_stdout("invalid password at line %u\n", line);
      break;
    case USERS_INVALID_ACCESS_LEVEL:
      log_to_stdout("invalid access level at line %u\n", line);
      break;
    case USERS_MAX_USERS_REACHED:
      log_to_stdout("max users reached\n");
      break;
    case USERS_WRONG_USERNAME:
      log_to_stdout("wrong username at line %u\n", line);
      break;
    case USERS_WRONG_PASSWORD:
      log_to_stdout("wrong password at line %u\n", line);
      break;
    case USERS_IO_ERROR:
      log_to_stdout("IO error at line %u\n", line);
      break;
    case USERS_NOMEM_ERROR:
      log_to_stdout("no memory available while loading users\n");
      break;
    case USERS_USER_ALREADY_EXISTS:
      log_to_stdout("user already exists at line %u\n", line);
      break;
    case USERS_UNKOWN_ERROR:
      log_to_stdout("unknown error at line %u\n", line);
      break;
    case USERS_OK:
    case USERS_USER_NOT_FOUND:
      break;
    }

  } while (result >= 0);

  fclose(file);
  return USERS_OK;
}

user_status users_init(char *users_file_param) {
  users_file_path = (users_file_param != NULL && users_file_param[0] != '\0')
                        ? users_file_param
                        : DEFAULT_USERS_FILE_PATH;

  users = malloc(sizeof(user_t) * MAX_USERS_REGISTERED);
  if (users == NULL) {
    return USERS_NOMEM_ERROR;
  }
  load_users(users_file_path);
  return USERS_OK;
}

user_status user_create(char *username, char *password,
                        access_level_t access_level) {
  if (username == NULL) {
    log_to_stdout("username cannot be null\n");
    return USERS_INVALID_USERNAME;
  }
  if (password == NULL) {
    log_to_stdout("password cannot be null\n");
    return USERS_INVALID_PASSWORD;
  }
  if (access_level != ADMIN && access_level != USER)
    return USERS_INVALID_ACCESS_LEVEL;
  if (users_size >= MAX_USERS_REGISTERED)
    return USERS_MAX_USERS_REACHED;
  if (get_user_index(username) >= 0) {
    log_to_stdout("user already exists\n");
    return USERS_USER_ALREADY_EXISTS;
  }
  strcpy(users[users_size].username, username);
  strcpy(users[users_size].password, password);
  users[users_size].access_level = access_level;
  users_size++;
  return USERS_OK;
}

user_status user_login(char *username, char *password,
                       access_level_t *output_level) {
  int i;
  if ((i = get_user_index(username)) < 0) {
    log_to_stdout("user %s not found\n", username);
    return USERS_USER_NOT_FOUND;
  } else {
    if (strcmp(users[i].password, password) == 0) {
      log_to_stdout("user %s logged in successfully\n", username);
      *output_level = users[i].access_level;
      return USERS_OK;
    }
    log_to_stdout("wrong password for user %s\n", username);
    return USERS_WRONG_PASSWORD;
  }
}

user_status valid_user(char *username, char *password) {
  char *c = username;
  while (c != NULL) {
    if (*c < MINIMUN_PRINTABLE_ASCII) {
      return USERS_UNKOWN_ERROR;
    }
  }
  c = password;
  while (c != NULL) {
    if (*c < MINIMUN_PRINTABLE_ASCII) {
      return USERS_UNKOWN_ERROR;
    }
  }
  return USERS_OK;
}

user_status user_change_password(char *username, char *password) {
  if (username == NULL) {
    return USERS_INVALID_USERNAME;
  }
  if (password == NULL) {
    return USERS_INVALID_PASSWORD;
  }
  int index;
  if ((index = get_user_index(username)) < 0) {
    return USERS_USER_NOT_FOUND;
  }
  strcpy(users[index].password, password);
  return USERS_OK;
}

user_status user_delete(char *user_username_to_delete) {
  if (user_username_to_delete == NULL) {
    log_to_stdout("username cannot be NULL\n");
    return USERS_INVALID_USERNAME;
  }
  int to_delete_index;
  if ((to_delete_index = get_user_index(user_username_to_delete)) < 0) {
    log_to_stdout("user %s not found\n", user_username_to_delete);
    return USERS_INVALID_USERNAME;
  }
  memmove(&users[to_delete_index], &users[to_delete_index],
          (users_size - to_delete_index - 1) * sizeof(user_t));
  users_size--;
  return USERS_OK;
}

static user_status save_users() {
  if (users_size == 0) {
    return USERS_OK;
  }
  FILE *file = fopen(users_file_path, WRTIE_MODE);

  if (file == NULL)
    return USERS_IO_ERROR;
  int left = users_size, i = 0;
  while (left) {
    user_t current_user = users[i];
    fprintf(file, "%c%s:%s\n", current_user.access_level == ADMIN ? '@' : '#',
            current_user.username, current_user.password);
    i++;
    left--;
  }
  return USERS_OK;
}

user_status users_shutdown() {
  if (save_users() != USERS_OK) {
    log_to_stdout("error while saving users\n");
    return USERS_IO_ERROR;
  }
  free(users);
  users_size = 0;
  return USERS_OK;
}
