/*-----------------------------------------------------------------------------
  magic.h
  
  (c) Juan D. Martin 1995
  (c) CNM-US 1995

-----------------------------------------------------------------------------*/
#ifndef MAGIC_H
#define MAGIC_H

#include "regexp/regexp.h"

#define M_LONG		0x0
#define M_SHORT		0x1
#define M_BYTE		0x2
#define M_STRING	0x3
#define M_MODE		0x4
#define M_LMODE		0x5
#define M_BUILTIN	0x6
#define M_REGEXP	0x7
#define M_TYPE		0xF		/* Mask for all types. */
#define M_MASKED	0x10		/* Value is masked. */
#define M_EQ		(0x0<<5)
#define M_LT		(0x1<<5)
#define M_GT		(0x2<<5)
#define M_SET		(0x3<<5)
#define M_OR		(0x4<<5)
#define M_ANY		(0x5<<5)
#define M_OP		(0x7<<5)	/* Mask for operations. */

typedef union
{
    long number;
    char *string;
    regexp *expr;
} m_un;

typedef struct
{
    off_t offset;
    m_un value;
    long mask;
    int flags;
    char *message;
    int subtypes;
} hmagic;

#endif /* MAGIC_H */
