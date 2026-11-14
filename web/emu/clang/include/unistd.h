/* SPDX-License-Identifier: GPL-3.0-only */
/* (Skiff XENO) declarations only, for web/emu/build_clang.sh: the emulator's live code calls none of these */
#pragma once
typedef int pid_t;
pid_t fork(void); int usleep(unsigned); unsigned sleep(unsigned); int unlink(const char*); int dup2(int,int); int close(int);
int pipe(int*); long read(int, void*, unsigned long); long write(int, const void*, unsigned long); int _exit(int);
