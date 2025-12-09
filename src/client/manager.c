#include "../lib/logger/logger.h"
#include "../lib/selector/selector.h"
#include "include/stats.h"
#include <time.h>

#define MAX_FDS 10

int main() {

  struct selector_init sel_init = {
      .signal = 0,
      .select_timeout = {.tv_sec = 1, .tv_nsec = 0},
  };

  selector_init(&sel_init);

  fd_selector fds = selector_new(MAX_FDS);

  logger_initialize(fds);

  stats *current_stats = get_stats();

  if (current_stats == NULL) {
    log_to_stdout("The server is currently down\n");

    logger_destroy();
    selector_destroy(fds);
    selector_close();
    return 1;
  }

  char time_str[100];
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

  log_to_stdout("STATISTICS FOR SERVER - %s\n\n", time_str);

  log_to_stdout("Historic connections to the server: %d\n",
                current_stats->historic_connections);

  log_to_stdout("Current connections to the server: %d\n",
                current_stats->current_connections);

  log_to_stdout("Total bytes transferred from server to client: %d\n",
                current_stats->transferred_bytes);

  log_to_stdout("END OF STATISTICS, EXECUTE AGAIN FOR UPDATED INFORMATION\n");

  selector_destroy(fds);

  selector_close();

  return 0;
}