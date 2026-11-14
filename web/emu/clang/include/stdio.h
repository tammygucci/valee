/* SPDX-License-Identifier: GPL-3.0-only */
/* (Skiff XENO) declarations only, for web/emu/build_clang.sh: the emulator's live code calls none of these */
#pragma once
#include <stddef.h>
#include <stdarg.h>
typedef struct FILE FILE;
extern FILE *stdout, *stderr, *stdin;
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_END 2
int printf(const char *, ...); int fprintf(FILE *, const char *, ...); int sprintf(char *, const char *, ...);
int snprintf(char *, size_t, const char *, ...); int vsnprintf(char *, size_t, const char *, va_list);
int puts(const char *); int putchar(int); int fputs(const char *, FILE *); int fputc(int, FILE *);
FILE *fopen(const char *, const char *); int fclose(FILE *); size_t fwrite(const void *, size_t, size_t, FILE *);
size_t fread(void *, size_t, size_t, FILE *); int fflush(FILE *); int fseek(FILE *, long, int); long ftell(FILE *);
int sscanf(const char *, const char *, ...); char *fgets(char *, int, FILE *); int remove(const char *);
int getc(FILE*); int feof(FILE*); void perror(const char*); FILE *popen(const char*, const char*); int pclose(FILE*);
