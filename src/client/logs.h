#ifdef DEBUG
#include <stdio.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif