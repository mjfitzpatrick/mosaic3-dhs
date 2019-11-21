/*******************************************************************************
 * 
 * __doc__ \section {The mnsnStd <<VERSION>> Common Defines and Macros}
 * __doc__ \subsection {mnsnStd.h.h}
 * __doc__ \begin{description}
 * __doc__  \item[\sc use:] \emph{\#include ``mnsnStd.h''}
 * __doc__  \item[\sc description:] this file contains all defines and macros
 * __doc__    common to all of the MONSOON software. It insures that commonly 
 * __doc__    concepts like TRUE/FALSE, YES/NO and OK/ERROR have common definitions.
 * __doc__    The file also defines size macros for messages, names and command lines 
 * __doc__    to help prevent memory overruns in string processing.
 * __doc__    A set of macros are provided for formatting monsoon status return strings.
 * __doc__  \item[\sc argument(s):]  not applicable
 * __doc__  \item[\sc return(s):] not applicable
 * __doc__  \item[\sc last modified:] Monday, 4 November 2002
 * __doc__  \item[\sc author(s):] Nick Buchholz (ncb), Phil Daly (pnd)
 * __doc__  \item[\sc license:] (c) 2002 AURA Inc. All rights reserved, Released under the GPL.
 * __doc__ \end{description} 
 *
 ******************************************************************************/
extern char __simStr[10];

#if !defined(_MNSN_STD_HDR_)
#define _MNSN_STD_HDR_

#define NO      0
#define YES     (!NO)

#define FALSE   0
#define TRUE    (!FALSE)

#define WARNING   +1
#define OK 	   0
#define ERROR     -1

#define MNSN_EXIT 	+1617614272
#define MNSN_SHTDWN	+1111638592

#if !defined(NULL)   /* 97/01/15 wtm - added ifndef because of conflict w/Solaris */
 #define	NULL		0
#endif
#define	CNULL		(char *) 0

#define MAXLINE 	1024
#define CLI_MAXLINE	320
#define MAXNAME		128

#define MAXMSG          4096	/* longest dhe message OK 8 bytes but this allows for expansion */
#define MAXSTRING	4096
#define MAXBUFS          256	
#define BADBUFINDEX	-1

#define ANAMESZ		256
#define MAXINFO	    ANAMESZ
#define     MAXUNITS        4
#define     MAXDHEUNITS        4

#define SCREENLINE	80
#define FNAMESZ		1024
#define EXACT   	-1

#define LESS    -1
#define GREATER  1
#define EQUAL    0

#define GET		0x00	/* get attribute value from DHE/PAN if OK from PAN otherwise */
#define SET		0x01	/* set attribute value in DHE/PAN if perms OK FAIL if not */
#define CHK_ACCESS	0x02	/* is it OK to read or write to DHE */
#define CHK_RANGE	0x03	/* is the value being written OK? */
#define OVRDSET		0x0F	/* set attribute value in DHE/PAN override prohibitions */

#if defined(_ACCESSLIST_)
 static char *accessList[] = { "GET","SET","CHK_ACCESS","CHK_RANGE","OVRDSET"};
#endif

#if defined(_DHESTATELIST_)
 static char *dheStateList[] = { "IDLE","RESET","EXP_ACTIVE","EXP_PAUSED","DATAXFR_LOCK","LASTXFR_LOCK"};
#endif

#if defined(_PANSTATELIST_)
 static char *panStateList[] = { "IDLE","EXP_ACTIVE","EXP_PAUSED","DATAXFR_LOCK","LASTXFR_LOCK", "ABORTING","STOPPING", "EXP_COMPLETE", "ERROR"};
#endif
#if defined(_DETMETHODS_)
 static char *detMethods[] = { "NOMETHOD","SIMPLE","INTTIME","ROISETC","ROISETR","FNAMESET", "SETVOFF","SETBIASV","SETFBIAS","WRT2READ","STRINGSET","RDMSKWRT", "SETDIGAVG" };
#endif
#if defined(_DHESIZES_)
 static char *dheSizes[] = { "ONEBIT(1 Bit)","BYTE(8 Bits)","CHAR(8 Bits)","UCHAR(8 Bits)","ULONG(32 Bits)","LONG(32 Bits)","SHORT(16 Bits)", "USHORT(16 Bits)","TWLVBIT(12 Bits)", "TWNT4BIT(24 Bits)" };
#endif

/* macros to access attribute value data union */
#define DVAL dValue.data
#define PVAL pValue.data

/* defines for open system call modes */
#define READ		0
#define WRITE		1
#define READ_WRITE	2

/* library code valid states */
#define MNSN_STATE_UNKNOWN	     -1L
#define MNSN_NOT_INITIALIZED	     -1L
#define MNSN_NOT_OPENED		     -1L
#define MNSN_INITIALIZED              1L
#define MNSN_OPENED                   2L
#define MNSN_CLOSED                   3L
#define MNSN_LINK_DOWN                4L
#define MNSN_SOCK_DOWN                4L
#define MNSN_LINK_UP                  5L
#define MNSN_LINK_FREE                6L
#define MNSN_LINK_LOCKED              7L 
#define MAGIC_NUMBER             0xFFFF0000

/* dhe state constants */
#define	DHE_DOWN 	0
#define DHE_READY	1
#define DHE_LOCKED	2
#define DHE_RESETING	3

typedef unsigned char	uchar;
typedef int		chHandle;
typedef int		dheHandle;

/*
#if !defined(int32_t)
 typedef int		int32_t;
#endif
#if !defined(uint32_t)
 typedef unsigned int	uint32_t;
#endif
*/

#define	or		else if
#define	forever		for(;;)
#define	until(expr)	while(!(expr))

#define	max(a, b)	((a) > (b) ? (a) : (b))
#define	min(a, b)	((a) < (b) ? (a) : (b))

#define streq(s1, s2)		(strcmp(s1, s2) == 0)
#define strdiff(s1, s2)		(strcmp(s1, s2) != 0)
#define strneq(s1, s2, n)	(strncmp(s1, s2, n) == 0)
#define strndiff(s1, s2, n)	(strncmp(s1, s2, n) != 0)

#define await_event(x)		{ while(!(x)) ; }

#if !defined(VALID_DHE_HNDL)
#define VALID_DHE_HNDL(_hndl)		((dhe_sMemP != NULL) && ((_hndl>=0) && (_hndl<MAXDHEUNITS)) && dhe_sMemP->cStruct[_hndl].state == MNSN_OPENED)
#define VALID_ADDR(_adr)		((_adr >= 0x0000) && (_adr<=0xFFFF))
#endif

#if !defined(VALID_COMH_HNDL)
#define VALID_COMH_HNDL(_hndl)		((com_sMemP != NULL) && ((_hndl>=0) && (_hndl<MAXCOMUNITS)) && (com_sMemP->cStruct[_hndl].link == MNSN_OPENED))
#endif

#define MNSN_CLEAR_STR(_S,_V) { \
 if ( (void *)(_S) != (void *)NULL ) (void) memset((_S),(_V),sizeof((_S))); \
}

#if 0
#define MNSN_PREFIX_SIM(_sim,_str,_statStr) { \
 if ( (_sim)==TRUE && (char *)(_str)!=(char *)NULL ) \
	(void) sprintf(&(_str)[strlen((_str))], "%s%s: %s", (_statStr),__simStr); \
 else (void) sprintf(&(_str)[strlen((_str))], "%s", _statStr); \
}

#define MNSN_WRITE_PTR(_str,_stt,_msg,_val) { \
 if ( (char *)(_str) != (char *)NULL ) (void) sprintf(&(_str)[strlen((_str))],"%s%s: %s (%p)\n",(_stt),__simStr,(_msg),(_val)); \
}

#define MNSN_WRITE_MSG(_str,_stt,_msg,_txt)   { if ( (char *)(_str) != (char *)NULL ) sprintf(&(_str)[strlen((_str))],"%s%s: %s %s\n",(_stt),__simStr,(_msg),(char *)(_txt)); }

#define MNSN_MSG(_str,_stt)   { if ( (char *)(_str) != (char *)NULL ) sprintf(&(_str)[strlen((_str))],"%s\n",(_stt)); }
#endif


#define MNSN_WRITE_VAL(_str,_stt,_msg,_val) { \
 if ( (char *)(_str) != (char *)NULL ) (void) sprintf(&(_str)[strlen((_str))],"%s%s: %s (%d)\n",(_stt),__simStr,(_msg),(int)(_val)); \
}

#define MNSN_REPORT(_F,_M) { \
 { \
  char __ts[20]; \
  char __rs[MAXLINE]; \
  (void) memset(__ts,'\0',20); \
  (void) memset(__rs,'\0',MAXLINE); \
  (void) sprintf(__rs,"%s @ %s\n",(_M),pTime(__ts)); \
  mnsnReport((_F),(char *)__rs); \
 } \
} 

#define MNSN_RPRT(_F,_M) { \
 if ( (char *)(_M)!=(char *)NULL && strlen(_M) ) { (void) fprintf((_F),"%s\n",(_M)); fflush(stderr); } \
}

#define MNSN_RET_SUCCESS(_S,_M) { \
    { \
	char __ts[20];				\
	char __rs[MAXLINE];			\
	(void) memset(__ts,'\0',20);		\
	(void) memset(__rs,'\0',MAXLINE);			\
	if ( (char *)(_S) != (char *)NULL && (strlen(_M) > 0 )) \
        {								\
	    (void) sprintf(__rs,"%s %s",(_M),(_S));			\
	    (void) strncpy((_S),__rs,MAXLINE-1);			\
	} \
    }	   \
}	   \


#define MNSN_RPT_SUCCESS(_S,_M) { \
    char __ts[20]; \
    (void) memset(__ts,'\0',20);					\
    (void) fprintf(stderr, "OK%s: %s %s\n",__simStr,(_M),(_S)); \
    (void) fflush(stderr);						\
}

#define MNSN_RET_FAILURE(_S,_M) { \
    { \
	char __ts[20];				\
	char __rs[MAXLINE];			\
	(void) memset(__ts,'\0',20);		\
	(void) memset(__rs,'\0',MAXLINE);	\
	if ( (char *)(_S) != (char *)NULL  && (strlen(_M) > 0 ) ) {	\
	    (void) sprintf(__rs,"%s %s",(_M),(_S));			\
	    (void) strncpy((_S),__rs,MAXLINE-1);			\
	    (void) fprintf(stderr,"<FAILURE> %s @ %s\n",(_S),pTime(__ts)); \
	    (void) fflush(stderr);					\
	}								\
    }									\
}

#define MNSN_RPT_FAILURE(_S,_M) { \
    char __ts[20]; \
    (void) memset(__ts,'\0',20);					\
    (void) fprintf(stderr, "ERROR%s: %s %s @ %s\n",__simStr,(_M),(_S),pTime(__ts)); \
    (void) fflush(stderr);						\
}							

#define MNSN_REPORT_MSG(_S,_M) { \
 if ( (char *)(_S) != (char *)NULL ) { \
  (void) memset((_S),'\0',sizeof((_S))); \
  if ( strlen((_M)) ) (void) sprintf((_S),"%s",(_M)); \
  (void) fprintf(stderr, "<M> %s\n", (_S)); \
  (void) fflush(stderr); \
 } \
}

#else
/* already included don't do it again */
#endif

