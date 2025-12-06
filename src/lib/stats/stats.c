#include "stats.h"
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define STATS_SHARED_MEMORY "/server_stats"
#define SHARED_MEMORY_PERMS 0666

static stats * current_stats = NULL;

static void init_stats() {
    if(current_stats != NULL) {
        return; // si ya inicialize la memoria compartida, no lo repito
    }

    // intento de abrir la memoria compartida SIN la opcion de creacion
    int shm_fd = shm_open(STATS_SHARED_MEMORY, O_RDWR, SHARED_MEMORY_PERMS);
    bool newly_created = false;
    
    if(shm_fd == -1) {
        // no esta creada, asi que lo hago
        shm_fd = shm_open(STATS_SHARED_MEMORY, O_CREAT | O_RDWR, SHARED_MEMORY_PERMS);
        newly_created = true;
        
        if(shm_fd == -1) {
            perror("shm_open");
            return;
        }
        
        ftruncate(shm_fd, sizeof(stats));
    }
    ftruncate(shm_fd, sizeof(stats));

    current_stats = mmap(NULL, sizeof(stats), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (current_stats == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        return;
    }

    if(newly_created) {
        memset(current_stats, 0, sizeof(stats));
    }
    close(shm_fd); // cierro ya que no lo voy a necesitar, ya hice el mappeo
}

void increment_current_connections() {
    init_stats();
    if(current_stats != NULL) {
        current_stats->historic_connections++;
        current_stats->current_connections++;
    }
}

void decrement_current_connections() {
    init_stats();
    if(current_stats != NULL) {
        current_stats->current_connections--;
    }
}

void add_transferred_bytes(size_t bytes_amount) {
    init_stats();
    if(current_stats != NULL) {
        current_stats->transferred_bytes += bytes_amount;
    }
}

stats * get_stats() {
    if (current_stats != NULL) {
        return current_stats;
    }

    // intento de abrir la memoria compartida. Si falla, es porque no esta, y por ende
    // uso el output NULL de la funcion para imprimir disinto en mi manager.
    int shm_fd = shm_open(STATS_SHARED_MEMORY, O_RDWR, SHARED_MEMORY_PERMS);
    if(shm_fd == -1) {
        return NULL;
    }

    current_stats = mmap(NULL, sizeof(stats), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (current_stats == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        current_stats = NULL;
        return NULL;
    }
    
    close(shm_fd);
    return current_stats;
}

void cleanup_stats() {
    shm_unlink(STATS_SHARED_MEMORY);
}

