/* SPDX-License-Identifier: GPL-3.0-only */
/* (Skiff XENO) declarations only, for web/emu/build_clang.sh: the emulator's live code calls none of these */
#pragma once
#include <stddef.h>
void *malloc(size_t); void *calloc(size_t, size_t); void *realloc(void *, size_t); void free(void *);
void exit(int); void abort(void); int atoi(const char *); long strtol(const char *, char **, int);
unsigned long strtoul(const char *, char **, int); double atof(const char *); double strtod(const char*, char**);
int abs(int); long labs(long); int rand(void); void srand(unsigned); char *getenv(const char *);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *)); int system(const char *);
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 2147483647
