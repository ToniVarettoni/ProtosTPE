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

  // Parse password
  int password_length = 0;
  while ((c = fgetc(users_file)) >= 0) {
    if (c < 32 || password_length > MAX_PASSWORD_LEN) {
      // invalid char at line :line
      if (c != '\n')
        return skipLine(users_file, line);
      (*line)++;
      return 1;
    }
    parsed_user->password[password_length++] = c;
  }
  parsed_user->password[password_length] = '\0';
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
    case INVALID_USERNAME:
      printf("invalid username at line %u", line);
      break;
    case INVALID_PASSWORD:
      printf("invalid password at line %u", line);
      break;
    case INVALID_ACCESS_LEVEL:
      printf("invalid acces level at line %u", line);
      break;
    case MAX_USERS_REACHED:
      printf("max users reached.");

      break;
    }
  } while (result >= 0);

  fclose(file);
  return OK;
}

user_status users_init(char *users_file_param) {
  users_file_path = (users_file_param != NULL && users_file_param[0] != '\0')
                        ? users_file_param
                        : DEFAULT_USERS_FILE_PATH;

  users = malloc(sizeof(user_t) * MAX_USERS);

  load_users(users_file_path);
}

user_status user_create(char *username, char *password,
                        access_level_t access_level) {
  if (username == NULL) {
    printf("username cannot be null\n");
    return INVALID_USERNAME;
  }
  if (password == NULL) {
    printf("password cannot be null\n");
    return INVALID_PASSWORD;
  }
  if (access_level != ADMIN && access_level != USER)
    return INVALID_ACCESS_LEVEL;
  if (users_size >= MAX_USERS)
    return MAX_USERS_REACHED;
  if (get_user_index(username) >= 0) {
    printf("user already exists");
    return USER_ALREADY_EXISTS;
  }
  strcpy(users[users_size].username, username);
  strcpy(users[users_size].password, password);
  users[users_size].access_level = access_level;
  users_size++;
  return 0;
}

user_status user_login(char *username, char *password,
                       access_level_t *output_level) {
  int i;
  if ((i = get_user_index(username)) < 0) {
    printf("user %s not found", username);
    return USER_NOT_FOUND;
  } else {
    if (strcmp(users[i].password, password) == 0) {
      printf("user %s logged in successfully", username);
      *output_level = users[i].access_level;
      return OK;
    }
    printf("wrong password for user %s", username);
    return WRONG_PASSWORD;
  }
}

user_status user_delete(char *user_username_to_delete,
                        char *user_username_who_deletes) {
  // to be implemented...
}

static user_status save_users() {
  FILE *file = fopen(users_file_path, WRTIE_MODE);

  if (file == NULL)
    return IO_ERROR;
  int left = users_size, i = 0;
  while (left) {
    user_t current_user = users[i];
    fprintf(file, "%c%s:%s\n", current_user.access_level == ADMIN ? '@' : '#',
            current_user.username, current_user.password);
    i++;
    left--;
  }
  return OK;
}

user_status users_shutdown() {
  if (save_users() != OK) {
    printf("error while saving users\n");
    return IO_ERROR;
  }
  free(users);
  users_size = 0;
  return OK;
}
