/*
 * rpl_printf.h
 *
 * Created: 28/02/2018 12:33:06 PM
 *  Author: teddy
 */ 

#pragma once

#ifdef __cplusplus

extern "C" {
	//int rpl_vsnprintf(char *, size_t, const char *, va_list);
	//int rpl_vasprintf(char **, const char *, va_list);
	//int rpl_asprintf(char **, const char *, ...);
	int rpl_snprintf(char *, size_t, const char *, ...) __attribute__((format(printf, 3, 4)));
}

#endif
