#ifndef LOGGER_H
#define LOGGER_H

#include "selector.h"

// Crea el logger a usar, cerando un FD nuevo (copia de STDOUT) no bloqueante
// para poder escribir.
void logger_initialize(fd_selector selector_param);

// Logguea directamente en linea de comandos. Se usa como un reemplazador de
// printf, pero asegurandose de no blockearse.
void log_to_stdout(char * format, ...);

// Se asegura de liberar todos los recursos utilizados para armar el logger.
void logger_destroy();

#endif