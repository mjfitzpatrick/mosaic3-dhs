#include <stdio.h>
#include <tcl.h>
#include "dso.h"
#include "kwdb.h"

/*
 * KWDBTCL.C -- Tcl interface to the KWDB keyword database library.
 *
 * The calling program should call the following routine to initialize
 * these routines:
 *
 *	      kwdb_TclInit (tcl)
 *
 * This installs the Tcl-callable KWDB interface in the given Tcl interpreter.
 * The Tcl-callable KWDB interface routines are as follows.
 *
 *         kwdb = kwdb_Open kwdbname
 *         name = kwdb_Name kwdb
 *    	     nkw = kwdb_Len kwdb
 *    	         kwdb_Close kwdb
 *    
 *    	      kwdb_AddEntry kwdb keyword [value [type [[comment]]]
 *    value = kwdb_GetValue kwdb keyword
 *            kwdb_SetValue kwdb keyword value
 *    	    kwdb_SetComment kwdb keyword comment
 *    str = kwdb_GetComment kwdb keyword
 *      type = kwdb_GetType kwdb keyword
 *             kwdb_SetType kwdb keyword type
 *    
 *         ep = kwdb_Lookup kwdb keyword [instance]
 *	     ep = kwdb_Head kwdb
 *	     ep = kwdb_Tail kwdb
 *	     ep = kwdb_Next kwdb ep
 *    	   kwdb_DeleteEntry kwdb ep
 *    	   kwdb_RenameEntry kwdb ep newname
 *           kwdb_CopyEntry kwdb o_kwdb o_ep [newname]
 *    count = kwdb_GetEntry kwdb ep keyword [value [type [comment]]]
 *	 name = kwdb_KWName kwdb ep
 *
 * These routines operate on a KWDB object in memory.  One can open and
 * close (destroy) the keyword database (KWDB), and read and write keywords
 * to the database.  See the KWDB source for additional details.
 *
 *     kwdb = kwdb_OpenFITS filename [maxcards [nblank]]
 *          kwdb_UpdateFITS kwdb filename [update [extend [npad] ]]
 *
 * These utility routines create a new KWDB and load the header of a FITS
 * file into it (OpenFITS), and save a KWDB to a new FITS file or update the
 * header of an existing file (UpdateFITS).  These are the only routines in
 * KWDB which know anything about FITS, the rest of the interface provides a
 * general purpose symbol table with special characteristics of FIFO ordering,
 * non-keyword entries, and comment lines on keywords.
 */

struct kwContext {
	int dummy;		/* not used at present */
};
typedef struct kwContext KwContext;

static int kwdbOpen(), kwdbName(), kwdbLen(), kwdbClose();
static int kwdbAddEntry(), kwdbGetValue(), kwdbSetValue(), kwdbGetType();
static int kwdbSetType(), kwdbSetComment(), kwdbGetComment();
static int kwdbLookup(), kwdbHead(), kwdbTail(), kwdbNext(), kwdbDeleteEntry();
static int kwdbRenameEntry(), kwdbCopyEntry(), kwdbGetEntry(), kwdbKWName();
static int kwdbOpenFITS(), kwdbUpdateFITS();

/* kwdb_TclInit -- Initialize the Tcl interface to KWDB.
 */
kwdb_TclInit (tcl)
register Tcl_Interp *tcl;
{
	KwContext *kw;
	kw = (KwContext *) calloc (1, sizeof(KwContext));
	if (!kw)
	    return (-1);

	Tcl_CreateCommand (tcl, "kwdb_Open", kwdbOpen, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_Name", kwdbName, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_Len", kwdbLen, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_Close", kwdbClose, kw, NULL);

	Tcl_CreateCommand (tcl, "kwdb_AddEntry", kwdbAddEntry, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_GetValue", kwdbGetValue, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_SetValue", kwdbSetValue, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_GetType", kwdbGetType, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_SetType", kwdbSetType, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_SetComment", kwdbSetComment, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_GetComment", kwdbGetComment, kw, NULL);

	Tcl_CreateCommand (tcl, "kwdb_Lookup", kwdbLookup, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_Head", kwdbHead, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_Tail", kwdbTail, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_Next", kwdbNext, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_DeleteEntry", kwdbDeleteEntry, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_RenameEntry", kwdbRenameEntry, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_CopyEntry", kwdbCopyEntry, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_GetEntry", kwdbGetEntry, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_KWName", kwdbKWName, kw, NULL);

	Tcl_CreateCommand (tcl, "kwdb_OpenFITS", kwdbOpenFITS, kw, NULL);
	Tcl_CreateCommand (tcl, "kwdb_UpdateFITS", kwdbUpdateFITS, kw, NULL);

	return (0);
}


/* kwdb_Open -- Open (create) a new keyword database.
 *
 * Usage:	kwdb = kwdb_Open kwdbName
 */
static int
kwdbOpen (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char buf[128];
	char *kwdbName;
	pointer kwdb;

	if (argc < 2)
	    return (TCL_ERROR);
	kwdbName = argv[1];

	if (!(kwdb = kwdb_Open (kwdbName)))
	    return (TCL_ERROR);

	sprintf (buf, "0x%x", kwdb);
	Tcl_SetResult (tcl, buf, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_Close -- Close a new keyword database.  This destroys the database
 * and frees all resources.
 *
 * Usage:	kwdb_Close kwdb
 */
static int
kwdbClose (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	pointer kwdb;

	if (argc < 2)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);

	kwdb_Close (kwdb);
	return (TCL_OK);
}


/* kwdb_Name -- Return the name of a keyword database.
 *
 * Usage:	name = kwdb_Name kwdb
 */
static int
kwdbName (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *kwdbName;
	pointer kwdb;

	if (argc < 2)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);

	if (!(kwdbName = kwdb_Name (kwdb)))
	    return (TCL_ERROR);

	Tcl_SetResult (tcl, kwdbName, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_Len -- Return the number of entries in a keyword database.
 *
 * Usage:	name = kwdb_Len kwdb
 */
static int
kwdbLen (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char buf[128];
	pointer kwdb;

	if (argc < 2)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);

	sprintf (buf, "%d", kwdb_Len(kwdb));
	Tcl_SetResult (tcl, buf, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_AddEntry -- Add an entry to a keyword database.
 *
 * Usage:	kwdb_AddEntry kwdb keyword [value [type [comment]]]
 */
static int
kwdbAddEntry (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword, *value, *type, *comment;
	char buf[128];
	pointer kwdb;
	int ep;

	if (argc < 3)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	keyword = argv[2];
	value = (argc >= 4) ? argv[3] : "";
	type = (argc >= 5) ? argv[4] : "";
	comment = (argc >= 6) ? argv[5] : "";

	if ((ep = kwdb_AddEntry (kwdb, keyword, value, type, comment)) < 0)
	    return (TCL_ERROR);

	sprintf (buf, "%d", ep);
	Tcl_SetResult (tcl, buf, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_GetValue -- Get the (string) value of a keyword.
 *
 * Usage:	value = kwdb_GetValue kwdb keyword
 */
static int
kwdbGetValue (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword, *value;
	pointer kwdb;

	if (argc < 3)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	keyword = argv[2];

	if (!(value = kwdb_GetValue (kwdb, keyword)))
	    return (TCL_ERROR);

	Tcl_SetResult (tcl, value, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_SetValue -- Set the value of a keyword.
 *
 * Usage:	kwdb_SetValue kwdb keyword value
 */
static int
kwdbSetValue (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword, *value;
	pointer kwdb;

	if (argc < 4)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	keyword = argv[2];
	value = argv[3];

	if (kwdb_SetValue (kwdb, keyword, value) < 0)
	    return (TCL_ERROR);

	return (TCL_OK);
}


/* kwdb_GetType -- Get the (string) type of a keyword.
 *
 * Usage:	type = kwdb_GetType kwdb keyword
 */
static int
kwdbGetType (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword, *type;
	pointer kwdb;

	if (argc < 3)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	keyword = argv[2];

	if (!(type = kwdb_GetType (kwdb, keyword)))
	    return (TCL_ERROR);

	Tcl_SetResult (tcl, type, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_SetType -- Set the type of a keyword.
 *
 * Usage:	kwdb_SetType kwdb keyword type
 */
static int
kwdbSetType (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword, *type;
	pointer kwdb;

	if (argc < 4)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	keyword = argv[2];
	type = argv[3];

	if (kwdb_SetType (kwdb, keyword, type) < 0)
	    return (TCL_ERROR);

	return (TCL_OK);
}


/* kwdb_GetComment -- Get the comment field of a keyword.
 *
 * Usage:	string = kwdb_GetComment kwdb keyword
 */
static int
kwdbGetComment (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword, *comment;
	pointer kwdb;

	if (argc < 3)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	keyword = argv[2];

	if (!(comment = kwdb_GetComment (kwdb, keyword)))
	    return (TCL_ERROR);

	Tcl_SetResult (tcl, comment, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_SetComment -- Set the comment field of a keyword.
 *
 * Usage:	kwdb_SetComment kwdb keyword comment
 */
static int
kwdbSetComment (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword, *comment;
	pointer kwdb;

	if (argc < 4)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	keyword = argv[2];
	comment = argv[3];

	if (kwdb_SetComment (kwdb, keyword, comment) < 0)
	    return (TCL_ERROR);

	return (TCL_OK);
}


/* kwdb_Lookup -- Lookup a keyword and return its entry pointer.
 *
 * Usage:	ep = kwdb_Lookup kwdb keyword [instance]
 *
 * If there are multiple values for the same keyword in the database then the
 * instance number determines which is returned.  An instance of zero returns
 * the first (most recently entered) instance.  Successive instances return
 * entries earlier in the list.  Note that this is opposite of an ordered
 * traversal of the list using kwdb_Head/kwdb_Next, which accesses the list
 * with oldest entries first.
 */
static int
kwdbLookup (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword;
	char buf[128];
	pointer kwdb;
	int instance, ep;

	if (argc < 3)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	keyword = argv[2];
	instance = (argc >= 4) ? atoi(argv[3]) : 0;

	if (!(ep = kwdb_Lookup (kwdb, keyword, instance)))
	    return (TCL_ERROR);

	sprintf (buf, "%d", ep);
	Tcl_SetResult (tcl, buf, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_Head -- Return the head of the keyword list.
 *
 * Usage:	ep = kwdb_Head kwdb
 */
static int
kwdbHead (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword;
	char buf[128];
	pointer kwdb;
	int ep;

	if (argc < 2)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);

	if (!(ep = kwdb_Head (kwdb)))
	    return (TCL_ERROR);

	sprintf (buf, "%d", ep);
	Tcl_SetResult (tcl, buf, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_Tail -- Return the head of the keyword list.
 *
 * Usage:	ep = kwdb_Tail kwdb
 */
static int
kwdbTail (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword;
	char buf[128];
	pointer kwdb;
	int ep;

	if (argc < 2)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);

	if (!(ep = kwdb_Tail (kwdb)))
	    return (TCL_ERROR);

	sprintf (buf, "%d", ep);
	Tcl_SetResult (tcl, buf, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_Next -- Given an entry pointer return the entry pointer of the next
 * element in the keyword list.  Zero is returned at the end of the list. 
 *
 * Usage:	ep = kwdb_Next kwdb ep
 */
static int
kwdbNext (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword;
	char buf[128];
	pointer kwdb;
	int o_ep, ep;

	if (argc < 3)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	o_ep = atoi (argv[2]);

	ep = kwdb_Next (kwdb, o_ep);
	sprintf (buf, "%d", ep);
	Tcl_SetResult (tcl, buf, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_DeleteEntry -- Delete a database entry (not necessarily a keyword)
 * given its entry pointer.
 *
 * Usage:	kwdb_DeleteEntry kwdb ep
 */
static int
kwdbDeleteEntry (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword;
	pointer kwdb;
	int ep;

	if (argc < 3)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	ep = atoi (argv[2]);

	if (kwdb_DeleteEntry (kwdb, ep) < 0)
	    return (TCL_ERROR);

	return (TCL_OK);
}


/* kwdb_RenameEntry -- Rename a database entry (not necessarily a keyword)
 * given its entry pointer.
 *
 * Usage:	kwdb_RenameEntry kwdb ep newname
 *
 * Either the old or new name can be nil.  Renaming an entry does not change
 * its position in the list but if there are redefinitions it becomes the
 * most recent instance.
 */
static int
kwdbRenameEntry (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *newname;
	pointer kwdb;
	int ep;

	if (argc < 4)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	ep = atoi (argv[2]);
	newname = argv[3];

	if (kwdb_RenameEntry (kwdb, ep, newname) < 0)
	    return (TCL_ERROR);

	return (TCL_OK);
}


/* kwdb_CopyEntry -- Copy an entry from one KWDB to another, given the
 * entry pointer of the element in the input keyword list.
 *
 * Usage:	ep = kwdb_CopyEntry kwdb o_kwdb o_ep [newname]
 *
 * If the newname argument is given it will be the name of the entry in the
 * new database.  The input and output databases can be the same.
 */
static int
kwdbCopyEntry (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *keyword, *newname;
	pointer kwdb, o_kwdb;
	char buf[128];
	int o_ep, ep;

	if (argc < 4)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	o_kwdb = (pointer) strtol (argv[2], NULL, 16);
	o_ep = atoi (argv[3]);
	newname = (argc >= 5) ? argv[4] : NULL;

	if (!(ep = kwdb_CopyEntry (kwdb, o_kwdb, o_ep, newname)))
	    return (TCL_ERROR);

	sprintf (buf, "%d", ep);
	Tcl_SetResult (tcl, buf, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_GetEntry -- Return all fields of a database entry given its entry
 * pointer.
 *
 * Usage:	count = kwdb_GetEntry kwdb ep keyword [value [type [comment]]]
 *
 * Count is the number of non-null fields in the entry (1-4).
 */
static int
kwdbGetEntry (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *a_keyword, *a_value, *a_type, *a_comment;
	char *keyword, *value, *type, *comment;
	char buf[128];
	pointer kwdb;
	int count, ep;

	if (argc < 4)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	ep = atoi (argv[2]);
	a_keyword = (argc >= 4) ? argv[3] : NULL;
	a_value = (argc >= 5) ? argv[4] : NULL;
	a_type = (argc >= 6) ? argv[5] : NULL;
	a_comment = (argc >= 7) ? argv[6] : NULL;

	if ((count = kwdb_GetEntry (kwdb, ep,
		&keyword, &value, &type, &comment)) < 0)
	    return (TCL_ERROR);

	if (a_keyword)
	    Tcl_SetVar (tcl, a_keyword, keyword, 0);
	if (a_value)
	    Tcl_SetVar (tcl, a_value, value, 0);
	if (a_type)
	    Tcl_SetVar (tcl, a_type, type, 0);
	if (a_comment)
	    Tcl_SetVar (tcl, a_comment, comment, 0);

	sprintf (buf, "%d", count);
	Tcl_SetResult (tcl, buf, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_KWName -- Return the name of a keyword given its entry pointer.
 *
 * Usage:	name = kwdb_KWName kwdb ep
 */
static int
kwdbKWName (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	pointer kwdb;
	char *name;
	int ep;

	if (argc < 3)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	ep = atoi (argv[2]);

	if ((name = kwdb_KWName (kwdb, ep)) == NULL)
	    return (TCL_ERROR);

	Tcl_SetResult (tcl, name, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_OpenFITS -- Open (create) a new keyword database and read a FITS file
 * header into it.
 *
 * Usage:	kwdb = kwdb_OpenFITS filename [maxcards [nblank]]
 *
 * "filename" should be a valid FITS file.  The FITS header excluding the END
 * card is read into the keyword database.  Maxcards, if given, specifies the
 * maximum number of cards to be read (one can use this to read just the first
 * few cards to get the primary keywords from a header).  Each header card of
 * any type is one entry in the database.  kwdb_Len will return the number of
 * lines (cards) in the FITS header.  Values of the type field generated are
 * L (logical), S (string), N (numeric), H (history), C (comment), T (text
 * other than history or comment), and B (blank).  Any trailing blank lines
 * at the end of the header are omitted, but a count of the number of blank
 * trailers will be returned in the "nblank" variable if given.
 */
static int
kwdbOpenFITS (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char buf[128];
	char *filename, *a_nblank;
	int maxcards, nblank;
	pointer kwdb;

	if (argc < 2)
	    return (TCL_ERROR);
	filename = argv[1];
	maxcards = (argc >= 3) ? atoi(argv[2]) : 0;
	a_nblank = (argc >= 4) ? argv[3] : NULL;

	if (!(kwdb = kwdb_OpenFITS (filename, maxcards, &nblank)))
	    return (TCL_ERROR);

	if (a_nblank) {
	    sprintf (buf, "%d", nblank);
	    Tcl_SetVar (tcl, a_nblank, buf, 0);
	}

	sprintf (buf, "0x%x", kwdb);
	Tcl_SetResult (tcl, buf, TCL_VOLATILE);

	return (TCL_OK);
}


/* kwdb_UpdateFITS -- Write or update a FITS file header from the contents of
 * a keyword database.
 *
 * Usage:	kwdb_UpdateFITS kwdb filename [update [extend [npad] ]]
 *
 * If the update flag is set the header of the FITS file "filename", which
 * must exist, is updated (replaced) from the contents of database KWDB.
 * If the update flag is not set a new (dataless) FITS file "filename" is
 * created from the contents of KWDB.  To get a valid FITS file the KWDB
 * must contain all the required FITS keywords.  If the header is being
 * udpated and there is insufficient space, and the "extend" flag is given,
 * the file header will be extended to make space (by shifting the file
 * contents to make space).  If the file has to be extended and a value
 * has been given for "npad", space for at least NPAD blank lines will be
 * added at the end of the header.
 */
static int
kwdbUpdateFITS (kw, tcl, argc, argv)
KwContext *kw;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	pointer kwdb;
	char *filename;
	int update, extend, npad;

	if (argc < 3)
	    return (TCL_ERROR);
	kwdb = (pointer) strtol (argv[1], NULL, 16);
	filename = argv[2];

	update = ((argc >= 4) && !strcmp(argv[3],"update"));
	extend = ((argc >= 5) && !strcmp(argv[4],"extend"));
	npad   = (argc >= 6) ? atoi(argv[5]) : 0;

	if (kwdb_UpdateFITS (kwdb, filename, update, extend, npad) < 0)
	    return (TCL_ERROR);

	return (TCL_OK);
}
