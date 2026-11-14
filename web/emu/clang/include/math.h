/* SPDX-License-Identifier: GPL-3.0-only */
/* (Skiff XENO) declarations only, for web/emu/build_clang.sh: the emulator's live code calls none of these */
#pragma once
#define M_PI 3.14159265358979323846
#define INFINITY __builtin_inff()
#define NAN __builtin_nanf("")
#define HUGE_VAL __builtin_huge_val()
#define isnan(x) __builtin_isnan(x)
#define isinf(x) __builtin_isinf(x)
#define isfinite(x) __builtin_isfinite(x)
#define signbit(x) __builtin_signbit(x)
double sin(double); double cos(double); double tan(double); double atan(double); double atan2(double, double);
double asin(double); double acos(double);
double exp(double); double log(double); double log10(double); double log2(double); double pow(double, double);
double sqrt(double); double fabs(double); double floor(double); double ceil(double); double round(double);
double fmod(double, double); double tanh(double); double sinh(double); double cosh(double); double exp2(double);
double trunc(double); double hypot(double,double); double cbrt(double); double expm1(double); double log1p(double);
long lround(double); long lrint(double); double rint(double); double nearbyint(double); double ldexp(double,int);
double frexp(double,int*); double modf(double,double*); double copysign(double,double); double fmax(double,double); double fmin(double,double);
float sinf(float); float cosf(float); float tanf(float); float expf(float); float logf(float); float powf(float, float);
float sqrtf(float); float fabsf(float); float floorf(float); float ceilf(float); float roundf(float); float tanhf(float);
float fmodf(float, float); float log10f(float); float log2f(float); float exp2f(float); float atanf(float); float atan2f(float,float);
long lroundf(float); float truncf(float); float rintf(float); long lrintf(float); float fmaxf(float,float); float fminf(float,float);
float copysignf(float,float); float hypotf(float,float); float asinf(float); float acosf(float); float sinhf(float); float coshf(float);
