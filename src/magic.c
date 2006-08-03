/*-----------------------------------------------------------------------------
  Module magic.c

  (c) Juan D. Martin 1995
  (c) CNM-US 1995
                                                                           
  magic headers related code.
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "magic.h"

/* ULTRIX apparently doesn't define these */
#ifdef ultrix
#define S_ISLNK(mode) (mode & S_IFMT) == S_IFLNK
#define S_ISSOCK(mode) (mode & S_IFMT) == S_IFSOCK
#endif

#define MAXLIN 1024
#define ASCLEN 512	
#define REGLEN 256	

static char    linebuf[MAXLIN];
static hmagic *mtypes = 0;	/* Array of file-types. */
static int     count = 0;	/* Number of types registered. */
static int     allocated = 0;	/* Room in the array. */
static int     top = 0;		/* Current top for sub-types. */
static int     maxhdr = ASCLEN;	/* Maximum header size required. */
static char   *hdrbuf = 0;	/* Header buffer allocated. */
static int     hdrbufsiz = 0;	/* Size of header buffer. */
static struct stat stbuf;	/* Stat buffer. */
static struct stat lstbuf;	/* Lstat buffer. */
static int bytes;		/* Bytes read from the file. */
static int     mmatch();
static char   *builtin_test();
static char   *builtin_result;
static char   *parse_string();

void magic_parse_file(name)
char *name;
{
    FILE *fh;
    int hsiz;

    if(!hdrbuf)
    {
	hdrbuf = (char *) malloc(maxhdr + 1);
	hdrbufsiz = maxhdr;
    }

    if(!name || !(fh = fopen(name, "r")))
	return;

    while(fgets(linebuf, MAXLIN, fh))
    {
	char *cptr, *sptr;
	int cnt = 0;
	int l;

	l = strlen(linebuf);
	while(l < MAXLIN && linebuf[--l] == '\n' && linebuf[--l] == '\\')
	{
	    if(!fgets(linebuf + l, MAXLIN - l, fh))
		break;
	    l = strlen(linebuf);
	}

	if(count >= allocated)
	{
	    if(!mtypes)
	    {
		allocated = 25;
		mtypes = (hmagic *) malloc(allocated * sizeof(hmagic));
	    }
	    else
	    {
		allocated += 25;
		mtypes = (hmagic *) realloc((char *) mtypes,
					    allocated * sizeof(hmagic));
	    }
	}

	cptr = linebuf;
	if(cptr[0] == '>')	/* Sub-type */
	{
	    cptr++;
	    mtypes[top].subtypes++;
	}
	else if(cptr[0] == '#' || cptr[0] == '\n' || cptr[0] == '\r')
	{
	    continue;
	}
	else
	{
	    top = count;
	    mtypes[top].subtypes = 0;
	}

	mtypes[count].offset = strtol(cptr, &cptr, 0);
	while(isspace(*cptr))
	    cptr++;

	while(islower(cptr[cnt]))
	    cnt++;

	if(!strncmp("string", cptr, cnt))
	{
	    mtypes[count].flags = M_STRING;
	    cptr += cnt;

	    sptr = parse_string(&cptr);
	    mtypes[count].mask = strlen(sptr);
	    mtypes[count].value.string = 
		strcpy((char *)malloc(mtypes[count].mask + 1), sptr);

	    hsiz = mtypes[count].offset + mtypes[count].mask;
	    if(hsiz > maxhdr)
		maxhdr = hsiz;
	}
	else if(!strncmp("builtin", cptr, cnt))
	{
	    mtypes[count].flags = M_BUILTIN;
	    cptr += cnt;

	    sptr = parse_string(&cptr);
	    mtypes[count].value.string = 
		strcpy((char *)malloc(strlen(sptr) + 1), sptr);
	}
	else if(!strncmp("regexp", cptr, cnt))
	{
	    mtypes[count].flags = M_REGEXP;
	    cptr += cnt;


	    if(*cptr == '&')
	    {
		cptr++;
		mtypes[count].mask = strtol(cptr, &cptr, 0);
	    }
	    else
		mtypes[count].mask = REGLEN;
	    hsiz = mtypes[count].offset + mtypes[count].mask;
	    if(hsiz > maxhdr)
		maxhdr = hsiz;

	    sptr = parse_string(&cptr);
	    mtypes[count].value.expr = regcomp(sptr);
	}
	else
	{
	    if(!strncmp("byte", cptr, cnt))
	    {
		mtypes[count].flags = M_BYTE;
		hsiz = mtypes[count].offset + 1;
		if(hsiz > maxhdr)
		    maxhdr = hsiz;
	    }
	    else if(!strncmp("short", cptr, cnt))
	    {
		mtypes[count].flags = M_SHORT;
		hsiz = mtypes[count].offset + 2;
		if(hsiz > maxhdr)
		    maxhdr = hsiz;
	    }
	    else if(!strncmp("long", cptr, cnt))
	    {
		mtypes[count].flags = M_LONG;
		hsiz = mtypes[count].offset + 4;
		if(hsiz > maxhdr)
		    maxhdr = hsiz;
	    }
	    else if(!strncmp("mode", cptr, cnt))
	    {
		mtypes[count].flags = M_MODE;
	    }
	    else if(!strncmp("lmode", cptr, cnt))
	    {
		mtypes[count].flags = M_LMODE;
	    }
	    else
		continue;		/* Error. Skip line. */

	    cptr += cnt;

	    if(*cptr == '&')
	    {
		mtypes[count].flags |= M_MASKED;
		cptr++;
		mtypes[count].mask = strtol(cptr, &cptr, 0);
	    }

	    while(isspace(*cptr))
		cptr++;
	    switch(*cptr)
	    {
	    case '=':
		mtypes[count].flags |= M_EQ;
		cptr++;
		break;
	    case '<':
		mtypes[count].flags |= M_LT;
		cptr++;
		break;
	    case '>':
		mtypes[count].flags |= M_GT;
		cptr++;
		break;
	    case '&':
		mtypes[count].flags |= M_SET;
		cptr++;
		break;
	    case '^':
		mtypes[count].flags |= M_OR;
		cptr++;
		break;
	    case 'x':
		mtypes[count].flags |= M_ANY;
		cptr++;
		break;
	    default:
		mtypes[count].flags |= M_EQ;
	    }
	    mtypes[count].value.number = strtol(cptr, &cptr, 0);
	}
	while(isspace(*cptr))
	    cptr++;
	sptr = cptr;
	while(*cptr != '\n' && *cptr != '\r' && *cptr != '\0')
	    cptr++;
	*cptr = '\0';
	mtypes[count].message = strcpy((char *)malloc(strlen(sptr) + 1), sptr);
	count++;
    }
    fclose(fh);

    if(maxhdr > hdrbufsiz)
    {
	free(hdrbuf);
	hdrbuf = (char *) malloc(maxhdr + 1);
	hdrbufsiz = maxhdr;
    }
}

void magic_get_type(name, buf)
char *name;
char *buf;
{
    int i;
    int fd;

    if(lstat(name, &lstbuf) < 0)
    {
	stbuf.st_mode = 0;
	lstbuf.st_mode = 0;
	bytes = -1;
    }
    else
    {
	if(S_ISLNK(lstbuf.st_mode))
	{
	    if(stat(name, &stbuf) < 0)
	    {
		stbuf.st_mode = 0;
		bytes = -1;
	    }
	}
	else
	    stbuf = lstbuf;
	if(S_ISREG(stbuf.st_mode))
	{
	    if(stbuf.st_size == 0)
		bytes = 0;
	    if((fd = open(name, O_RDONLY, 0)) < 0)
		bytes = -1;
	    else
	    {
		bytes = read(fd, hdrbuf, maxhdr);
		close(fd);
		/* Make sure it is nul-terminated. */
		if(bytes >= 0)
		    hdrbuf[bytes] = '\0'; 
	    }
	}
	else
	    bytes = 0;
    }

    builtin_result = (char *)0;
    for(i = 0; i < count; i++)
    {
	if(mmatch(i, buf))
	{
	    if(mtypes[i].subtypes != 0)
	    {
		char *nbuf;
		int n;

		if(buf[0] != '\0')
		    strcat(buf, " ");
		nbuf = buf + strlen(buf);
		n = i + mtypes[i].subtypes + 1;
		for(++i; i < n; i++)
		{
		    if(mmatch(i, nbuf) && nbuf[0] != '\0')
		    {
			strcat(nbuf, " ");
			nbuf = nbuf + strlen(nbuf);
		    }
		}
		n = strlen(buf) - 1;
		if(buf[n] == ' ')
		    buf[n] = '\0';
	    }
	    return;
	}
	else
	    i += mtypes[i].subtypes;
    }
    strcpy(buf, builtin_test());
}

static int mmatch(i, buf)
int i;
char *buf;
{
    int t;
    int o;
    long v;
    unsigned char *h;

    h = (unsigned char *) (hdrbuf + mtypes[i].offset);
    t = mtypes[i].flags & M_TYPE;

    switch(t)
    {
    case M_STRING:
	if(bytes >= mtypes[i].offset + mtypes[i].mask &&
	   !strncmp(hdrbuf + mtypes[i].offset, 
		    mtypes[i].value.string,
		    mtypes[i].mask))
	{
	    strcpy(buf, mtypes[i].message);
	    return 1;
	}
	return 0;

    case M_BUILTIN:
	if(!strcmp(mtypes[i].value.string, builtin_test()))
	{
	    strcpy(buf, mtypes[i].message);
	    return 1;
	}
	return 0;

    case M_REGEXP:
    {
	char sav;
	int len;
	regexp *prog = mtypes[i].value.expr;

	if(bytes <= mtypes[i].offset || !prog)
	    return 0;

	if(bytes > mtypes[i].offset + mtypes[i].mask)
	{
	    len = mtypes[i].mask;
	    sav = h[len];
	    h[len] = '\0';
	}
	else
	    len = -1;
	
	if(regexec(prog , h))
	{
	    regsub(prog, mtypes[i].message, buf);
	    if(len >= 0)
		h[len] = sav;
	    return 1;
	}
	if(len >= 0)
	    h[len] = sav;
    }
    return 0;

    case M_BYTE:
	if(bytes < mtypes[i].offset + 1)
	    return 0;
	else
	    v = h[0];
	break;

    case M_SHORT:
	if(bytes < mtypes[i].offset + 2)
	    return 0;
	else
	    v = (h[0] << 8) | h[1];
	break;

    case M_LONG:
	if(bytes < mtypes[i].offset + 4)
	    return 0;
	else
	    v = (h[0] << 24) | (h[1] << 16) | (h[2] << 8) | h[3];
	break;

    case M_MODE:
	v = stbuf.st_mode;
	break;

    case M_LMODE:
	v = lstbuf.st_mode;
	break;

    default:
	return 0;		/* Should never happen. */
    }

    if (mtypes[i].flags & M_MASKED)
	v &= mtypes[i].mask;

    o = mtypes[i].flags & M_OP;
    switch(o)
    {
    case M_EQ:
	if(v == mtypes[i].value.number)
	{
	    sprintf(buf, mtypes[i].message, v);
	    return 1;
	}
	else
	    return 0;
    case M_LT:
	if(v < mtypes[i].value.number)
	{
	    sprintf(buf, mtypes[i].message, v);
	    return 1;
	}
	else
	    return 0;
    case M_GT:
	if(v > mtypes[i].value.number)
	{
	    sprintf(buf, mtypes[i].message, v);
	    return 1;
	}
	else
	    return 0;
    case M_SET:
	if((v & mtypes[i].value.number) == mtypes[i].value.number)
	{
	    sprintf(buf, mtypes[i].message, v);
	    return 1;
	}
	else
	    return 0;
    case M_OR:
	if((v & mtypes[i].value.number) != 0)
	{
	    sprintf(buf, mtypes[i].message, v);
	    return 1;
	}
	else
	    return 0;
    case M_ANY:
	sprintf(buf, mtypes[i].message, v);
	return 1;
    }
    return 0;
}

static char *builtin_test()
{
    int i;

    if(builtin_result)
	return builtin_result;
    if(bytes > 0)
    {
	if(bytes > ASCLEN)
	    bytes = ASCLEN;
	for(i = 0; i < bytes; i++)
	    if(!isascii(hdrbuf[i]))
		return builtin_result = "data";
	return builtin_result = "ascii";
    }
    if (bytes == 0)
    {
	if(S_ISREG(stbuf.st_mode))
	    return builtin_result = "empty";
	else
	    return builtin_result = "special";
    }
    return builtin_result = "unreadable";
}

static char *parse_string(s)
char **s;
{
    char cbuf[4];
    int i;
    char *vptr, *sptr, *cptr;

    cptr = *s;
    while(isspace(*cptr))
	    cptr++;
    vptr = sptr = cptr;

    while(!isspace(*cptr))
    {
	if(*cptr != '\\')
	{
	    *vptr++ = *cptr++;
	    continue;
	}
	cptr++;
	switch(*cptr)
	{
	case '\\':
	    *vptr++ = '\\';
	    cptr++;
	    break;
	case 'n':
	    *vptr++ = '\n';
	    cptr++;
	    break;
	case 'r':
	    *vptr++ = '\r';
	    cptr++;
	    break;
	case 't':
	    *vptr++ = '\t';
	    cptr++;
	    break;
	case 'v':
	    *vptr++ = '\v';
	    cptr++;
	    break;
	case 'b':
	    *vptr++ = '\b';
	    cptr++;
	    break;
	case 'f':
	    *vptr++ = '\f';
	    cptr++;
	    break;
	case 'x':
	    cptr++;
	    for(i = 0; i < 3 && isxdigit(*cptr); i++, cptr++)
		cbuf[i] = *cptr;
	    cbuf[i] = '\0';
	    *vptr++ = (char) strtol(cbuf, 0, 16);
	    break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	    for(i = 0; i < 3 && *cptr >= '0' && *cptr <= '7';
		i++, cptr++)
		cbuf[i] = *cptr;
	    cbuf[i] = '\0';
	    *vptr++ = (char) strtol(cbuf, 0, 8);
	    break;
	default:
	    *vptr++ = *cptr++;
	}
    }
    *vptr = '\0';
    cptr++;
    *s = cptr;
    return sptr;
}
