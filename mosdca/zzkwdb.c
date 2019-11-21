#include <stdio.h>
#include "dso.h"
#include "kwdb.h"

main(argc,argv)
int argc;
char **argv;
{
	char *fname = argv[1];
	char *kw, *vl, *dt, *cm;
	int count, nblanks, ep;
	char keyword[128];
	pointer kwdb;

	if (argc < 2)
	    exit (1);

	if (!(kwdb = kwdb_OpenFITS (fname, &nblanks))) {
	    fprintf (stderr, "cannot open %s\n", fname);
	    exit (2);
	}

	printf ("opened %s, nentries = %d\n", fname, kwdb_Len(kwdb));

/*
	printf ("> "); fflush (stdout);
	while (gets (keyword)) {
	    printf ("%s = %s\n", keyword, kwdb_GetValue(kwdb,keyword));
	    ep = kwdb_Lookup (kwdb, keyword);
	    if (ep > 0) {
		count = kwdb_GetEntry (kwdb, ep, &kw, &vl, &dt, &cm);
		printf ("ep=%d: %s = %s (%s) %s\n", ep, kw, vl, dt, cm);
	    } else
		printf ("keyword %s not found\n", keyword);
	    printf ("> "); fflush (stdout);
	}
 */

	printf ("----- contents of database %s ---------\n", kwdb_Name(kwdb));
	for (ep=kwdb_Head(kwdb);  ep;  ep=kwdb_Next(kwdb,ep)) {
		count = kwdb_GetEntry (kwdb, ep, &kw, &vl, &dt, &cm);
		printf ("ep=%2d: %s = `%s' (%s) `%s'\n",
		    ep, kw, vl, dt, cm);
	}

	exit (0);
}
