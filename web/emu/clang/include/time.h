/* SPDX-License-Identifier: GPL-3.0-only */
/* (Skiff XENO) declarations only, for web/emu/build_clang.sh: the emulator's live code calls none of these */
#pragma once
typedef long time_t; typedef long clock_t;
struct timespec { time_t tv_sec; long tv_nsec; };
#define CLOCKS_PER_SEC 1000000L
#define CLOCK_MONOTONIC 1
clock_t clock(void); time_t time(time_t *); int clock_gettime(int, struct timespec *);
