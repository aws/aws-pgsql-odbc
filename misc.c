/*-------
 * Module:			misc.c
 *
 * Description:		This module contains miscellaneous routines
 *					such as for debugging/logging and string functions.
 *
 * Classes:			n/a
 *
 * API functions:	none
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *-------
 */

#include "psqlodbc.h"
#include "misc.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifndef WIN32
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <process.h>			/* Byron: is this where Windows keeps def.
								 * of getpid ? */
#endif

/*
 *	returns STRCPY_FAIL, STRCPY_TRUNCATED, or #bytes copied
 *	(not including null term)
 */
ssize_t
my_strcpy(char *dst, ssize_t dst_len, const char *src, ssize_t src_len)
{
	if (dst_len <= 0)
		return STRCPY_FAIL;

	if (src_len == SQL_NULL_DATA)
	{
		dst[0] = '\0';
		return STRCPY_NULL;
	}
	else if (src_len == SQL_NTS)
		src_len = strlen(src);

	if (src_len <= 0)
		return STRCPY_FAIL;
	else
	{
		if (src_len < dst_len)
		{
			memcpy(dst, src, src_len);
			dst[src_len] = '\0';
		}
		else
		{
			memcpy(dst, src, dst_len - 1);
			dst[dst_len - 1] = '\0';	/* truncated */
			return STRCPY_TRUNCATED;
		}
	}

	return strlen(dst);
}


/*
 * strncpy copies up to len characters, and doesn't terminate
 * the destination string if src has len characters or more.
 * instead, I want it to copy up to len-1 characters and always
 * terminate the destination string.
 */
size_t
strncpy_null(char *dst, const char *src, ssize_t len)
{
	int			i;

	if (NULL != dst && len > 0)
	{
		for (i = 0; src[i] && i < len - 1; i++)
			dst[i] = src[i];

		dst[i] = '\0';
	}
	else
		return 0;
	if (src[i])
		return strlen(src);
	return i;
}


/*------
 *	Create a null terminated string (handling the SQL_NTS thing):
 *		1. If buf is supplied, place the string in there
 *		   (assumes enough space) and return buf.
 *		2. If buf is not supplied, malloc space and return this string
 *------
 */
char *
make_string(const SQLCHAR *s, SQLINTEGER len, char *buf, size_t bufsize)
{
	size_t		length;
	char	   *str;

	if (!s || SQL_NULL_DATA == len)
		return NULL;
	if (len >= 0)
		length =len;
	else if (SQL_NTS == len)
		length = strlen((char *) s);
	else
	{
		MYLOG(MIN_LOG_LEVEL, "invalid length=" FORMAT_INTEGER "\n", len);
		return NULL;
	}
	if (buf)
	{
		strncpy_null(buf, (char *) s, bufsize > length ? length + 1 : bufsize);
		return buf;
	}

MYLOG(DETAIL_LOG_LEVEL, "malloc size=" FORMAT_SIZE_T "\n", length);
	str = malloc(length + 1);
MYLOG(DETAIL_LOG_LEVEL, "str=%p\n", str);
	if (!str)
		return NULL;

	strncpy_null(str, (char *) s, length + 1);
	return str;
}

char *
my_trim(char *s)
{
	char *last;

	for (last = s + strlen(s) - 1; last >= s; last--)
	{
		if (*last == ' ')
			*last = '\0';
		else
			break;
	}

	return s;
}

/*
 * snprintfcat is a extension to snprintf
 * It add format to buf at given pos
 */
#ifdef POSIX_SNPRINTF_REQUIRED
static posix_vsnprintf(char *str, size_t size, const char *format, va_list ap);
#define vsnprintf posix_vsnprintf
#endif /* POSIX_SNPRINTF_REQUIRED */

int
snprintfcat(char *buf, size_t size, const char *format, ...)
{
	int len;
	size_t pos = strlen(buf);
	va_list arglist;

	va_start(arglist, format);
	len = vsnprintf(buf + pos, size - pos, format, arglist);
	va_end(arglist);
	return len + pos;
}

/*
 * snprintf_len is a extension to snprintf
 * It returns strlen of buf every time (not -1 when truncated)
 */

size_t
snprintf_len(char *buf, size_t size, const char *format, ...)
{
	ssize_t len;
	va_list arglist;

	va_start(arglist, format);
	if ((len = vsnprintf(buf, size, format, arglist)) < 0)
		len = size;
	va_end(arglist);
	return len;
}

/*
 * Windows doesn't have snprintf(). It has _snprintf() which is similar,
 * but it behaves differently wrt. truncation. This is a compatibility
 * function that uses _snprintf() to provide POSIX snprintf() behavior.
 *
 * Our strategy, if the output doesn't fit, is to create a temporary buffer
 * and call _snprintf() on that. If it still doesn't fit, enlarge the buffer
 * and repeat.
 */
#ifdef POSIX_SNPRINTF_REQUIRED
static int
posix_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int			len;
	char	   *tmp;
	size_t		newsize;

	len = _vsnprintf(str, size, format, ap);
	if (len < 0)
	{
		if (size == 0)
			newsize = 100;
		else
			newsize = size;
		do
		{
			newsize *= 2;
			tmp = malloc(newsize);
			if (!tmp)
				return -1;
			len = _vsnprintf(tmp, newsize, format, ap);
			if (len >= 0)
				memcpy(str, tmp, size);
			free(tmp);
		} while (len < 0);
	}
	if (len >= size && size > 0)
	{
		/* Ensure the buffer is NULL-terminated */
		str[size - 1] = '\0';
	}
	return len;
}

int
posix_snprintf(char *buf, size_t size, const char *format, ...)
{
	int		len;
	va_list arglist;

	va_start(arglist, format);
	len = posix_vsnprintf(buf, size, format, arglist);
	va_end(arglist);
	return len;
}
#endif /* POSIX_SNPRINTF_REQUIRED */

#ifndef	HAVE_STRLCAT
size_t
strlcat(char *dst, const char *src, size_t size)
{
	size_t ttllen;
	char	*pd = dst;
	const char *ps= src;

	for (ttllen = 0; ttllen < size; ttllen++, pd++)
	{
		if (0 == *pd)
			break;
	}
	if (ttllen >= size - 1)
		return ttllen + strlen(src);
	for (; ttllen < size - 1; ttllen++, pd++, ps++)
	{
		if (0 == (*pd = *ps))
			return ttllen;
	}
	*pd = 0;
	for (; *ps; ttllen++, ps++)
		;
	return ttllen;
}
#endif /* HAVE_STRLCAT */


/*
 * Proprly quote and escape a possibly schema-qualified table name.
 */
char *
quote_table(const pgNAME schema, const pgNAME table, char *buf, int buf_size)
{
	const char *ptr;
	int			i;

	i = 0;

	if (NAME_IS_VALID(schema))
	{
		buf[i++] = '"';
		for (ptr = SAFE_NAME(schema); *ptr != '\0' && i < buf_size - 6; ptr++)
		{
			buf[i++] = *ptr;
			if (*ptr == '"')
				buf[i++] = '"';		/* escape quotes by doubling them */
		}
		buf[i++] = '"';
		buf[i++] = '.';
	}

	buf[i++] = '"';
	for (ptr = SAFE_NAME(table); *ptr != '\0' && i < buf_size - 3; ptr++)
	{
		buf[i++] = *ptr;
		if (*ptr == '"')
			buf[i++] = '"'; 		/* escape quotes by doubling them */
	}
	buf[i++] = '"';
	buf[i] = '\0';

	return buf;
}

char* hide_password(const char* str, const char end_char)
{
	char* outstr, * pwdp;

	if (!str)	return NULL;
	outstr = strdup(str);
	if (!outstr) return NULL;
	if (pwdp = strstr(outstr, "PWD="), !pwdp)
		pwdp = strstr(outstr, "pwd=");
	if (pwdp)
	{
		char* p;

		for (p = pwdp + 4; *p && *p != ';'; p++)
			*p = 'x';
	}

	if (pwdp = strstr(outstr, "PASSWORD="), !pwdp)
		pwdp = strstr(outstr, "password=");
	if (!pwdp)
		pwdp = strstr(outstr, "Password=");
	if (!pwdp)
		pwdp = strstr(outstr, "PassWord=");
	if (pwdp)
	{
		char* p;

		for (p = pwdp + 9; *p && *p != end_char; p++)
			*p = 'x';
	}

	return outstr;
}
