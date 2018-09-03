/*
 * xstring.h
 *
 *  Created on: Nov 17, 2015
 *      Author: root
 */

#ifndef XSTRING_H_
#define XSTRING_H_

#include <string.h>
#include <stdlib.h>

char* str_trim(char* str);

int str_eq(const char* str1, const char* str2);

int str_eqCase(const char* str1, const char* str2);

int str_startsWith(const char* str, const char* with);

int str_startsWithCase(const char* str, const char* with);

int str_endsWith(const char* str, const char* with);

int str_endsWithCase(const char* str, const char* with);

int str_contains(const char* str, const char* with);

int str_containsCase(const char* str, const char* with);

char* str_toLowerCase(char* str);

char* str_toUpperCase(char* str);

char* str_urlencode(char* str); // must be freed and str must be on heap

char* str_urldecode(char* str);

char* str_replace(char* str, char* from, char* to); // when strlen(to) > strlen(from), str MUST be heap allocated!

char* str_replaceCase(char* str, char* from, char* to);

int str_isUNum(const char* str);

char* str_dup(char* str, ssize_t expand);

#endif /* XSTRING_H_ */
