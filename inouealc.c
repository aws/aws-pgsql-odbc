#undef	_MEMORY_DEBUG_
#include	"psqlodbc.h"

#ifndef	_MIMALLOC_
#ifdef	WIN32
#ifdef	_DEBUG
/* #include	<stdlib.h> */
#define	_CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#else
#include	<malloc.h>
#endif /* _DEBUG */
#endif /* WIN32 */
#endif /* _MIMALLOC_ */
#include	<string.h>

#include	"misc.h"
typedef struct {
	size_t	len;
	void	*aladr;
} ALADR;

static int	alsize = 0;
static int	tbsize = 0;
static ALADR	*altbl = NULL;

CSTR	ALCERR	= "alcerr";
void * pgdebug_alloc(size_t size)
{
	void * alloced;
	alloced = pg_malloc(size);
MYLOG(DETAIL_LOG_LEVEL, " alloced=%p(" FORMAT_SIZE_T ")\n", alloced, size);
	if (alloced)
	{
		if (!alsize)
		{
			alsize = 100;
			altbl = (ALADR *) pg_malloc(alsize * sizeof(ALADR));
		}
		else if (tbsize >= alsize)
		{
			ALADR *al;
			alsize *= 2;
			if (al = (ALADR *) pg_realloc(altbl, alsize * sizeof(ALADR)), NULL == al)
				return alloced;
			altbl = al;
		}
		altbl[tbsize].aladr = alloced;
		altbl[tbsize].len = size;
		tbsize++;
	}
	else
		MYLOG(MIN_LOG_LEVEL, "%s:alloc " FORMAT_SIZE_T "byte\n", ALCERR, size);
	return alloced;
}
void * pgdebug_calloc(size_t n, size_t size)
{
	void * alloced = pg_calloc(n, size);

	if (alloced)
	{
		if (!alsize)
		{
			alsize = 100;
			altbl = (ALADR *) pg_malloc(alsize * sizeof(ALADR));
		}
		else if (tbsize >= alsize)
		{
			ALADR *al;
			alsize *= 2;
			if (al = (ALADR *) pg_realloc(altbl, alsize * sizeof(ALADR)), NULL == al)
				return alloced;
			altbl = al;
		}
		altbl[tbsize].aladr = alloced;
		altbl[tbsize].len = n * size;
		tbsize++;
	}
	else
		MYLOG(MIN_LOG_LEVEL, "%s:calloc " FORMAT_SIZE_T "byte\n", ALCERR, size);
MYLOG(DETAIL_LOG_LEVEL, "calloced = %p\n", alloced);
	return alloced;
}
void * pgdebug_realloc(void * ptr, size_t size)
{
	void * alloced;

	if (!ptr)
		return pgdebug_alloc(size);
	alloced = pg_realloc(ptr, size);
	if (!alloced)
	{
		MYLOG(MIN_LOG_LEVEL, "%s %p error\n", ALCERR, ptr);
	}
	else /* if (alloced != ptr) */
	{
		int	i;
		for (i = 0; i < tbsize; i++)
		{
			if (altbl[i].aladr == ptr)
			{
				altbl[i].aladr = alloced;
				altbl[i].len = size;
				break;
			}
		}
	}

	MYLOG(DETAIL_LOG_LEVEL, "%p->%p\n", ptr, alloced);
	return alloced;
}
char * pgdebug_strdup(const char * ptr)
{
	char * alloced = pg_strdup(ptr);
	if (!alloced)
	{
		MYLOG(MIN_LOG_LEVEL, "%s %p error\n", ALCERR, ptr);
	}
	else
	{
		if (!alsize)
		{
			alsize = 100;
			altbl = (ALADR *) pg_malloc(alsize * sizeof(ALADR));
		}
		else if (tbsize >= alsize)
		{
			ALADR *al;
			alsize *= 2;
			if (al = (ALADR *) pg_realloc(altbl, alsize * sizeof(ALADR)), NULL == al)
				return alloced;
			altbl = al;
		}
		altbl[tbsize].aladr = alloced;
		altbl[tbsize].len = strlen(ptr) + 1;
		tbsize++;
	}
	MYLOG(DETAIL_LOG_LEVEL, "%p->%p(%s)\n", ptr, alloced, alloced);
	return alloced;
}

void pgdebug_free(void * ptr)
{
	int i, j;
	int	freed = 0;

	if (!ptr)
	{
		MYLOG(MIN_LOG_LEVEL, "%s null ptr\n", ALCERR);
		return;
	}
	for (i = 0; i < tbsize; i++)
	{
		if (altbl[i].aladr == ptr)
		{
			for (j = i; j < tbsize - 1; j++)
			{
				altbl[j].aladr = altbl[j + 1].aladr;
				altbl[j].len = altbl[j + 1].len;
			}
			tbsize--;
			freed = 1;
			break;
		}
	}
	if (! freed)
	{
		MYLOG(MIN_LOG_LEVEL, "%s not found ptr %p\n", ALCERR, ptr);
		return;
	}
	else
		MYLOG(DETAIL_LOG_LEVEL, "ptr=%p\n", ptr);
	pg_free(ptr);
}

static BOOL out_check(void *out, size_t len, const char *name)
{
	BOOL	ret = TRUE;
	int	i;

	for (i = 0; i < tbsize; i++)
	{
		if ((ULONG_PTR)out < (ULONG_PTR)(altbl[i].aladr))
			continue;
		if ((ULONG_PTR)out < (ULONG_PTR)(altbl[i].aladr) + altbl[i].len)
		{
			if ((ULONG_PTR)out + len > (ULONG_PTR)(altbl[i].aladr) + altbl[i].len)
			{
				ret = FALSE;
				MYLOG(MIN_LOG_LEVEL, "%s:%s:out_check found memory buffer overrun %p(" FORMAT_SIZE_T ")>=%p(" FORMAT_SIZE_T ")\n", ALCERR, name, out, len, altbl[i].aladr, altbl[i].len);
			}
			break;
		}
	}
	return ret;
}
char *pgdebug_strcpy(char *out, const char *in)
{
	if (!out || !in)
	{
		MYLOG(MIN_LOG_LEVEL, "%s null pointer out=%p,in=%p\n", ALCERR, out, in);
		return NULL;
	}
	out_check(out, strlen(in) + 1, __FUNCTION__);
	return strcpy(out, in);
}
char *pgdebug_strncpy(char *out, const char *in, size_t len)
{
	if (!out || !in)
	{
		MYLOG(MIN_LOG_LEVEL, "%s null pointer out=%p,in=%p\n", ALCERR, out, in);
		return NULL;
	}
	out_check(out, len, __FUNCTION__);
	return strncpy(out, in, len);
}
char *pgdebug_strncpy_null(char *out, const char *in, size_t len)
{
	if (!out || !in)
	{
		MYLOG(MIN_LOG_LEVEL, "%s null pointer out=%p,in=%p\n", ALCERR, out, in);
		return NULL;
	}
	out_check(out, len, __FUNCTION__);
	strncpy_null(out, in, len);
	return out;
}

void *pgdebug_memcpy(void *out, const void *in, size_t len)
{
	if (!out || !in)
	{
		MYLOG(MIN_LOG_LEVEL, "%s null pointer out=%p,in=%p\n", ALCERR, out, in);
		return NULL;
	}
	out_check(out, len, __FUNCTION__);
	return memcpy(out, in, len);
}

void *pgdebug_memset(void *out, int c, size_t len)
{
	if (!out)
	{
		MYLOG(MIN_LOG_LEVEL, "%s null pointer out=%p\n", ALCERR, out);
		return NULL;
	}
	out_check(out, len, __FUNCTION__);
	return memset(out, c, len);
}

void debug_memory_check(void)
{
	int i;

	if (0 == tbsize)
	{
		MYLOG(MIN_LOG_LEVEL, "no memry leak found and max count allocated so far is %d\n", alsize);
		pg_free(altbl);
		alsize = 0;
	}
	else
	{
		MYLOG(MIN_LOG_LEVEL, "%s:memory leak found check count=%d alloc=%d\n", ALCERR, tbsize, alsize);
		for (i = 0; i < tbsize; i++)
		{
			MYLOG(MIN_LOG_LEVEL, "%s:leak = %p(" FORMAT_SIZE_T ")\n", ALCERR, altbl[i].aladr, altbl[i].len);
		}
	}
}
