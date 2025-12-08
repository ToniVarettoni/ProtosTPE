#ifndef _USERS_H_
#define _USERS_H_

#define DEFAULT_USERS_FILE_PATH "src/server/users.txt"

#define MAX_USERNAME_LEN 255
#define MAX_PASSWORD_LEN 255

#define MAX_USERS 50

// TODO: improve error handling

typedef enum {
  USERS_OK = 0,
  USERS_INVALID_ACCESS_LEVEL,
  USERS_WRONG_USERNAME,
  USERS_INVALID_USERNAME,
  USERS_WRONG_PASSWORD,
  USERS_INVALID_PASSWORD,
  USERS_USER_ALREADY_EXISTS,
  USERS_USER_NOT_FOUND,
  USERS_MAX_USERS_REACHED,
  USERS_IO_ERROR,
  USERS_NOMEM_ERROR,
  USERS_UNKOWN_ERROR
} user_status;

/* Users file will have the following structure representing a user with its
username, password and access level:
*  a '@' or '#'  representing an admin or an user respectively
*  username in plain text
*  password in plain text separated from username by a ':'
*  Example of an user with username "kick", password "buttowski" and access
level ADMIN:
@kick:buttowski
*/

typedef enum { USER = 1, ADMIN = 0 } access_level_t;

typedef struct {
  char username[MAX_USERNAME_LEN + 1];
  char password[MAX_PASSWORD_LEN + 1];
  access_level_t access_level;
} user_t;

user_status users_init(char *users_file_param);

user_status user_create(char *username, char *password,
                        access_level_t access_level);

user_status user_login(char *username, char *password,
                       access_level_t *output_level);

user_status user_delete(char *user_username_to_delete,
                        char *user_username_who_deletes);

user_status users_shutdown();

#endif