/* SPDX-License-Identifier: GPL-3.0-only */
/* (Skiff XENO) declarations only, for web/emu/build_clang.sh: the emulator's live code calls none of these */
#pragma once
#include <unistd.h>
pid_t waitpid(pid_t, int *, int); pid_t wait(int *);
#define WIFEXITED(s) 1
#define WEXITSTATUS(s) (s)
#define WIFSIGNALED(s) 0
#define WTERMSIG(s) 0
