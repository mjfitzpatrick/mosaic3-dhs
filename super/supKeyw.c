#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <ctype.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Tcl/tcl.h>

#include "super.h"


#define KEYW_DEBUG (getenv("KEYW_DEBUG")!=NULL||access("/tmp/KEYW_DEBUG",F_OK)==0)

extern	int console;


/* Keyword database structures.  The keyword database is simply an array of
** entries indexed by an exposure id.  Within each entry we maintain a linked
** list of keyw=value pairs.
*/

#define	SZ_SYMVAL	32

typedef struct {			/* an individual keyw=val pair	  */
    char name[SZ_SYMVAL];
    char value[SZ_SYMVAL];
    void *next;
} sym, *symPtr;


typedef struct {			/* an entry in the DB per keyword */
    double expID;
    symPtr dbHead;
    symPtr dbTail;
} kdbEntry, *kdbEntryPtr;

kdbEntryPtr	keywDB[MAX_KEYWMON];	/* keyword database		  */
int		keywDBTop	= 0;



/*****************************************************************************
**  Update the keyword monitoring list with the latest information.
*/
void
supUpdateKeywDB (char *list)
{
    int    i, found=0;
    char   *ip, *op, keyw[SZ_FNAME], val[SZ_FNAME];
    double expID;
    kdbEntryPtr entry;
    symPtr  new_sym, cur_tail;


    /* Extract the expID from the first element of the list string.
    */
    expID = atof (list);

    /* Now skip ahead to start of the keyw=val pairs by skipping the expID.
    */
    for (ip=list; *ip && !isspace(*ip); ip++) ;

    /* Now look up the expID in the database, allocate a new entry if it
    ** isn't found.
    */
    found = 0;
    for (i=0; i < keywDBTop; i++) {
	if (smcEqualExpID (expID, keywDB[i]->expID)) {
	    found++;
	    entry = keywDB[i];
	    break;
	}
    }
    if (!found) {
	/* Add to top of the stack. */
	keywDB[keywDBTop++] = entry = (kdbEntryPtr) calloc(1, sizeof(kdbEntry));
	entry->expID = expID;
	entry->dbHead = entry->dbTail = (symPtr) NULL;
    }

    /* Begin parsing the keyw=value pairs.  We assume these will be unique
    ** and come from multiple databases, so simply add them to the end of the
    ** list.
    */
    while (*ip) {

	/* Extract the keyw and value.
	*/
	memset (keyw, 0, SZ_FNAME);
	while (*ip && isspace(*ip)) 		/* skip leading white	*/
	    ip++;
	for (op=&keyw[0]; *ip && *ip != '='; )	/* get keyword		*/
	    *op++ = *ip++;
	ip += 2;				/* skip opening quote	*/
	memset (val, 0, SZ_FNAME);
	for (op=&val[0]; *ip && *ip != '\''; )	/* get value		*/
	    *op++ = *ip++;
	ip += 2;				/* skip closing quote	*/

	/* Allocate a new entry for the keyword and add it to the list.
	** If this is the first keyword, it has been allocated above so 
	** fill it in, otherwise, allocate a new one and reset pointers.
	*/
	new_sym = (symPtr) calloc (1, sizeof(sym));
	if (entry->dbHead == (symPtr) NULL) {	/* i.e. first keyword	*/
	    entry->dbHead = new_sym;
	    cur_tail = (symPtr) NULL;
	} else {
	    cur_tail = entry->dbTail;
	}

	strcpy (new_sym->name, keyw);
	strcpy (new_sym->value, val);
	new_sym->next = (symPtr) NULL;

	if (!cur_tail) {
	    entry->dbTail = new_sym;		/* create tail of list	*/
	} else {
	    cur_tail->next = new_sym;		/* connect to previous	*/
	    entry->dbTail = new_sym;		/* reset tail		*/
	}
    }

    printKeywList (expID);
}


void
printKeywList (double expID)
{
    register int i, check=0;
    kdbEntryPtr entry;
    symPtr  sym;


    /* Look up the expID in the database, print the keywords found.
    */
    for (i=0; i < keywDBTop; i++) {
	if (smcEqualExpID (expID, keywDB[i]->expID)) {
	    entry = keywDB[i];

	    /* Walk the list following the 'next' links from the head.
	    */
	    for (sym=entry->dbHead; sym; sym=sym->next) {
		printf ("%.6lf  %-24s %s\n", 
		    expID, sym->name, sym->value);

		if (check++ > MAX_KEYWMON)	/* safety check		*/ 
		    break;
	    }
	    break;
	}
    }
}



/*****************************************************************************
**  Make a list of the keywords to monitor.    
*/
char *
supMakeKeywordList ()
{
    register int  i;
    char buf[SZ_LINE];
    char *kw = calloc (1, SZ_LINE * MAX_KEYWMON);


    strcpy (kw, "keyword add ");
    for (i=0; i < NKeywords; i++) {
        sprintf (buf, "%s.%s ", keywList[i].dbname, keywList[i].keyw);
        strcat (kw, buf);
    }

    return (kw);
}
