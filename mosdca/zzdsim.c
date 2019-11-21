#include <stdio.h>
#include <sys/types.h>
#include "dso.h"
#include "dsim.h"

/*
 * ZZDSIM -- Perform basic tests of the DSIM interface.
 *
 *	Offset		Description
 *	-------		-----------
 *	      0		PHU
 *
 *	   5760		Extension 1 Header 
 *	  11520		Extension 1 Pixels
 *			517*782*2 = 808588 bytes -> 809280 aligned
 *			extnsize = 815040 bytes
 *
 *	 820800		Extension 2 Header
 *	 826560		Extension 2 Pixels
 *			512*1024*2 = 1048576 bytes -> 1051200 aligned
 *			extnsize = 1056960 bytes
 *
 *			filesize = 1877760 bytes
 * 
 */
main()
{
	register int i, j;
	register ushort *op;
	pointer dsim, kwdb, io;
	int image;

	dsim = dsim_Open ("test.fits", DSO_CREATE);
	if (!dsim)
	    exit (1);

	dsim_Set (dsim, 0, DSIM_NImages, 2);
	dsim_Set (dsim, 0, DSIM_MaxKeywords, 72);

	image = 1;
	dsim_Set (dsim, image, DSIM_MaxKeywords, 72);
	dsim_Set (dsim, image, DSIM_PixelType, DSO_USHORT);
	dsim_Set (dsim, image, DSIM_NAxes, 2);
	dsim_Set (dsim, image, DSIM_Axlen1, 517);
	dsim_Set (dsim, image, DSIM_Axlen2, 782);

	image = 2;
	dsim_Set (dsim, image, DSIM_MaxKeywords, 72);
	dsim_Set (dsim, image, DSIM_PixelType, DSO_USHORT);
	dsim_Set (dsim, image, DSIM_NAxes, 2);
	dsim_Set (dsim, image, DSIM_Axlen1, 512);
	dsim_Set (dsim, image, DSIM_Axlen2, 1024);

	if (dsim_Configure (dsim) < 0)
	    exit (2);

	/* Write initial global header. */
	image = 0;
	kwdb = dsim_ReadHeader (dsim, image, 0);
	kwdb_AddEntry (kwdb, "global1", "value", "S", "Test string");
	kwdb_AddEntry (kwdb, "global2", "T", "L", "Test logical");
	kwdb_AddEntry (kwdb, "foo", "123", "N", "Test numerical");
	kwdb_AddEntry (kwdb, "global3", "123", "N", "Test numerical");
	kwdb_AddEntry (kwdb, "comment", "this is a comment", "C", NULL);
	kwdb_AddEntry (kwdb, "history", "this is a history", "H", NULL);
	kwdb_DeleteEntry (kwdb, kwdb_Lookup(kwdb,"foo"));
	dsim_WriteHeader (dsim, image, kwdb);
	kwdb_Close (kwdb);

	/* Test pixel i/o.
	 */
	image = 1;
	{
	    int sv[2], nv[2];
	    int nx = dsim_Get (dsim, image, DSIM_Axlen1);
	    int ny = dsim_Get (dsim, image, DSIM_Axlen2);

	    sv[0] = 0;   sv[1] = 0;
	    nv[0] = nx;  nv[1] = ny;

	    io = dsim_StartIO (dsim, image, DSO_RDWR, sv, nv);
	    for (j=0;  j < nv[1];  j++) {
		op = (ushort *) dsim_Pixel2D (io, 0, sv[1]+j);
		for (i=0;  i < nv[0];  i++)
		    *op++ = j*10 + i;
	    }
	    dsim_FinishIO (io);
	}

	image = 2;
	{
	    int sv[2], nv[2];
	    int nx = dsim_Get (dsim, image, DSIM_Axlen1);
	    int ny = dsim_Get (dsim, image, DSIM_Axlen2);

	    sv[0] = 0;   sv[1] = 0;
	    nv[0] = nx;  nv[1] = ny;

	    io = dsim_StartIO (dsim, image, DSO_RDWR, sv, nv);
	    for (j=0;  j < nv[1];  j++) {
		op = (ushort *) dsim_Pixel2D (io, 0, sv[1]+j);
		for (i=0;  i < nv[0];  i++)
		    *op++ = j*10 + i;
	    }
	    dsim_FinishIO (io);
	}

	/* Update headers. */
	image = 0;
	kwdb = dsim_ReadHeader (dsim, image, 0);
	kwdb_AddEntry (kwdb, "global1", "value", "S", "Test string");
	kwdb_AddEntry (kwdb, "global2", "T", "L", "Test logical");
	kwdb_AddEntry (kwdb, "foo", "123", "N", "Test numerical");
	kwdb_AddEntry (kwdb, "global3", "123", "N", "Test numerical");
	kwdb_AddEntry (kwdb, "comment", "this is a comment", "C", NULL);
	kwdb_AddEntry (kwdb, "history", "this is a history", "H", NULL);
	kwdb_DeleteEntry (kwdb, kwdb_Lookup(kwdb,"foo"));
	dsim_WriteHeader (dsim, image, kwdb);
	kwdb_Close (kwdb);

	image = 1;
	kwdb = dsim_ReadHeader (dsim, image, 0);
	kwdb_AddEntry (kwdb, "name1", "value", "S", "Test string");
	kwdb_AddEntry (kwdb, "name2", "T", "L", "Test logical");
	kwdb_AddEntry (kwdb, "name3", "123", "N", "Test numerical");
	kwdb_AddEntry (kwdb, "comment", "this is a comment", "C", NULL);
	kwdb_AddEntry (kwdb, "history", "this is a history", "H", NULL);
	dsim_WriteHeader (dsim, image, kwdb);
	kwdb_Close (kwdb);

	image = 2;
	kwdb = dsim_ReadHeader (dsim, image, 0);
	kwdb_AddEntry (kwdb, "name1", "value", "S", "Test string");
	kwdb_AddEntry (kwdb, "name2", "T", "L", "Test logical");
	kwdb_AddEntry (kwdb, "name3", "123", "N", "Test numerical");
	kwdb_AddEntry (kwdb, "comment", "this is a comment", "C", NULL);
	kwdb_AddEntry (kwdb, "history", "this is a history", "H", NULL);
	dsim_WriteHeader (dsim, image, kwdb);
	kwdb_Close (kwdb);

	dsim_Close (dsim);
}
