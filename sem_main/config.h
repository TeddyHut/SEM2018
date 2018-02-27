/*
 * config.h
 *
 * Created: 18/01/2018 4:10:34 PM
 *  Author: teddy
 */ 

#pragma once

#if 1
#ifndef __cplusplus
#define vsnprintf rpl_vsnprintf
#define snprintf rpl_snprintf
#define vasprintf rpl_vasprintf
#define asprintf rpl_asprintf
#endif
#define HAVE_VSNPRINTF					0
#define HAVE_SNPRINTF					0
#define HAVE_VASPRINTF					0
#define HAVE_ASPRINTF					0
#define HAVE_STDARG_H					1
#define HAVE_STDDEF_H					1
#define HAVE_STDINT_H					1
#define HAVE_STDLIB_H					1
#define HAVE_FLOAT_H					1
#define HAVE_INTTYPES_H					1
#define HAVE_LOCALE_H					0
#define HAVE_LOCALECONV					0
#define HAVE_LCONV_DECIMAL_POINT		0
#define HAVE_LCONV_THOUSANDS_SEP		0
#define HAVE_LONG_DOUBLE				0
#define HAVE_LONG_LONG_INT				0
#define HAVE_UNSIGNED_LONG_LONG_INT		0
#define HAVE_INTMAX_T					1
#define HAVE_UINTMAX_T					1
#define HAVE_UINTPTR_T					1
#define HAVE_PTRDIFF_T					1
#define HAVE_VA_COPY					1
#define HAVE___VA_COPY					1
#define TEST_SNPRINTF					0
#define NEED_MEMCPY						0

#endif
