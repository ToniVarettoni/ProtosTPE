#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args_monitor.h"

static void usage(const char *progname) {
  fprintf(stderr,
          "Usage: %s <direction:port> [OPTION]... -l <user:pass>\n"
          "\n"
          "   <direction:port>  Destination address and port for the proxy "
          "server (required unless -h).\n"
          "   -l <user:pass>     Login credentials (required).\n"
          "   -a <user:pass>     Add a user with the provided password.\n"
          "   -d <user>          Delete a user.\n"
          "   -c <user:newpass>  Change the password for an existing user.\n"
          "   -s                 Fetch server statistics.\n"
          "   -h                 Print this help message and exit.\n"
          "\n",
          progname);
  exit(1);
}

static void ensure_single_action(management_args_t *args,
                                 management_action_t action) {
  if (args->action != ACTION_NONE && args->action != action) {
    fprintf(stderr, "Only one management action can be executed at a time.\n");
    exit(1);
  }

  args->action = action;
}

static unsigned short parse_port(const char *s) {
  char *end = 0;
  long sl = strtol(s, &end, 10);
  if (end == s || *end != '\0' || sl <= 0 || sl > 65535) {
    fprintf(stderr, "Port must be in range 1-65535.\n");
    exit(1);
  }
  return (unsigned short)sl;
}

static void parse_direction_port(char *arg, management_args_t *args) {
  char *sep = strchr(arg, ':');
  if (sep == NULL || sep == arg || *(sep + 1) == '\0') {
    fprintf(stderr,
            "Invalid direction/port. Expected format <direction>:<port>.\n");
    exit(1);
  }
  *sep = '\0';
  args->direction = arg;
  args->port = parse_port(sep + 1);
}

static user_t parse_userpass(char *arg, bool expect_access_level) {
  char *separator = strchr(arg, ':');
  if (separator == NULL || separator == arg || *(separator + 1) == '\0') {
    fprintf(stderr, "Invalid user:pass format.\n");
    exit(1);
  }

  *separator = '\0';
  char *username_out = arg;
  char *password_out = separator + 1;

  uint8_t access_level = 0;
  if (expect_access_level) {
    size_t passlen = strlen(password_out);
    if (passlen == 0) {
      fprintf(stderr, "Password cannot be empty.\n");
      exit(1);
    }
    char level = password_out[passlen - 1];
    if (level == '#') {
      access_level = 1; // USER
      password_out[passlen - 1] = '\0';
    } else if (level == '@') {
      access_level = 0; // ADMIN
      password_out[passlen - 1] = '\0';
    } else {
      fprintf(stderr, "Missing access level (# for user, @ for admin).\n");
      exit(1);
    }
  }

  user_t user_out = {
      .username = username_out,
      .password = password_out,
      .access_level = access_level,
  };

  return user_out;
}

static void login(management_args_t *args, char *user) {
  user_t logging_user = parse_userpass(user, false);

  args->managing_user = logging_user;
}

static void add_user_to_modify(management_args_t *args, char *user) {
  user_t added_user = parse_userpass(user, true);

  args->user_to_modify = added_user;
}

void parse_monitor_args(const int argc, char **argv, management_args_t *args) {
  memset(args, 0, sizeof(*args));
  args->action = ACTION_NONE;

  if (argc < 2) {
    fprintf(stderr,
            "Missing required direction and port (<direction>:<port>).\n");
    usage(argv[0]);
  }

  // Positional: direction:port must be immediately after the binary unless -h.
  if (argv[1][0] == '-') {
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
      usage(argv[0]);
    }
    fprintf(stderr,
            "Expected <direction>:<port> immediately after the binary.\n");
    usage(argv[0]);
  }
  parse_direction_port(argv[1], args);
  optind = 2;

  int c;
  while (true) {
    int option_index = 0;
    static struct option long_options[] = {{0, 0, 0, 0}};

    c = getopt_long(argc, argv, "l:a:d:c:sh", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'l':
      if (args->managing_user.username != NULL || args->managing_user.password != NULL) {
        fprintf(stderr, "Login (-l) provided more than once.\n");
        exit(1);
      }
      login(args, optarg);
      break;
    case 'a':
      ensure_single_action(args, ACTION_ADD_USER);
      add_user_to_modify(args, optarg);
      break;
    case 'd':
      ensure_single_action(args, ACTION_DELETE_USER);
      if (optarg == NULL || *optarg == '\0') {
        fprintf(stderr, "Username required for deleting a user.\n");
        exit(1);
      }
      args->user_to_modify.username = optarg;
      break;
    case 'c':
      ensure_single_action(args, ACTION_CHANGE_PASSWORD);
      add_user_to_modify(args, optarg);
      break;
    case 's':
      ensure_single_action(args, ACTION_STATS);
      break;
    case 'h':
      usage(argv[0]);
      break;
    default:
      fprintf(stderr, "Unknown argument %d.\n", c);
      exit(1);
    }
  }

  if (optind < argc) {
    fprintf(stderr, "Unexpected argument: ");
    while (optind < argc) {
      fprintf(stderr, "%s ", argv[optind++]);
    }
    fprintf(stderr, "\n");
    exit(1);
  }

  if (args->managing_user.username == NULL || args->managing_user.password == NULL) {
    fprintf(stderr, "Missing required login (-l <user:pass>).\n");
    usage(argv[0]);
  }

  if (args->action == ACTION_NONE) {
    fprintf(stderr, "No management action specified.\n");
    usage(argv[0]);
  }
}
