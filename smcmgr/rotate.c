/*
**  ROTATE.C -- Utility routines to flip or transpose raster arrays.
*/


typedef unsigned char uchar;
typedef unsigned short ushort;
#ifdef XLONG
typedef unsigned int ulong;
#else
typedef unsigned long ulong;
#endif

#define NULL 0
uchar  *ubip 	= (uchar *)  NULL;
ushort *usip 	= (ushort *) NULL;
ulong  *ulip 	= (ulong *)  NULL;
uchar  *ubop 	= (uchar *)  NULL;
ushort *usop 	= (ushort *) NULL;
ulong  *ulop 	= (ulong *)  NULL;

void flipx(int type, int nx, int ny);
void flipy(int type, int nx, int ny);
void transpose1(int type, int nx, int ny);
void transpose2(int type, int nx, int ny);

extern	void *calloc(), free();



/*  ROTATE -- Flip or transpose an input array.
**
**  	 iadr - Input array
**	 oadr - output array
**	 type - datatype (mainly 4)  1:uchar, 2:ushort, 4: ulong
**	 nx   - NAXIS1
**	 ny   - NAXIS2
**	 dir     1 - Flip x axis
**	 	 2 - Flip y axis
**	 	 3 - Transpose around the main diagonal '/'
**	 	 4 - Transpose around the 2nd diagonal  '\'
*/

void
rotate (void *iadr, XLONG **oadr, int type, int nx, int ny, int dir)
{

    int    nelem;

    static int first_time = 1, sv_nx=0, sv_ny=0;;
    static ulong  *bufl = (ulong *) NULL;
    static ushort *bufs = (ushort *) NULL;
    static uchar  *bufc = (uchar *) NULL;

    
    if ((nx*ny) != (sv_nx*sv_ny)) {
        if (type == 1 && bufc) free ((uchar *)bufc);
        if (type == 2 && bufs) free ((ushort *)bufs);
        if (type == 4 && bufl) free ((ulong *)bufl);
	sv_nx = nx; 
	sv_ny = ny;
	first_time = 1;
    }

    nelem = nx * ny;

    switch (type) {

    case (1):			/* uchar */
	ubip = (uchar *) iadr;
	if (first_time) {
	    ubop = bufc = (uchar *) calloc (1, nelem * sizeof(uchar));
	    *oadr = (XLONG *) ubop;
	    first_time = 0;
	} else {
	    ubop = (uchar *)bufc;
	    *oadr = (XLONG *) bufc;
	}
	break;
    case (2):			/* ushort */
	usip = (ushort *) iadr;
	if (first_time) {
	    usop = bufs = (ushort *) calloc (1, nelem * sizeof(ushort));
	    *oadr = (XLONG *) usop;
	    first_time = 0;
	} else {
	    usop = (ushort *)bufs;
	    *oadr = (XLONG *) bufs;
	}
	break;
    case (4):			/* ulong */
	ulip = (ulong *) iadr;
	if (first_time) {
	    ulop = bufl = (ulong *) calloc (1, nelem * sizeof(ulong));
	    *oadr = (XLONG *) ulop;
	    first_time = 0;
	} else {
	    ulop = (ulong *)bufl;
	    *oadr = (XLONG *) bufl;
	}
	break;
    default:
	;
	break;
    }

    switch (dir) {
    case (1):			/* flip x axis */
	flipx(type, nx, ny);
	break;
    case (2):			/* flip y axis */
	flipy(type, nx, ny);
	break;
    case (3):			/* transpose 1st diag */
	transpose1(type, nx, ny);
	break;
    case (4):			/* transpose 2nd diag */
	transpose2(type, nx, ny);
	break;
    default:
	break;
    }
}


void
flipx (int pixsize, int nx, int ny)
{
    int x, y;
    extern uchar *ubip, *ubop;
    extern ushort *usip, *usop;
    extern ulong *ulip, *ulop;

    switch (pixsize) {
    case (1):			/*uchar */
	for (y = 0; y < ny; y++) {
	    for (x = nx; x > 0; x--) {
		*ubop++ = *(ubip + y * nx + x - 1);
	    }
	}
	break;
    case (2):			/*ushort */
	for (y = 0; y < ny; y++) {
	    for (x = nx; x > 0; x--) {
		*usop++ = *(usip + y * nx + x - 1);
	    }
	}
	break;
    case (4):			/*ulong */
	for (y = 0; y < ny; y++) {
	    for (x = nx; x > 0; x--) {
		*ulop++ = *(ulip + y * nx + x - 1);
	    }
	}
	break;
    default:
	break;
    }
}


void
flipy (int pixsize, int nx, int ny)
{
    int x, y;
    extern uchar *ubip, *ubop;
    extern ushort *usip, *usop;
    extern ulong *ulip, *ulop;

    switch (pixsize) {
    case (1):			/*char */
	for (y = ny - 1; y >= 0; y--) {
	    for (x = 0; x < nx; x++) {
		*ubop++ = *(ubip + y * nx + x);
	    }
	}
	break;
    case (2):			/*ushort */
	for (y = ny - 1; y >= 0; y--) {
	    for (x = 0; x < nx; x++) {
		*usop++ = *(usip + y * nx + x);
	    }
	}
	break;
    case (4):			/*ulong */
	for (y = ny - 1; y >= 0; y--) {
	    for (x = 0; x < nx; x++) {
		*ulop++ = *(ulip + y * nx + x);
	    }
	}
	break;
    default:
	break;
    }
}


void
transpose2 (int pixsize, int nx, int ny)
{
    /* Transpose the array pointed by *u.ip along
       the 2nd diagonal (\). Output goes to *u.op
     */
    int x, y;
    extern uchar *ubip, *ubop;
    extern ushort *usip, *usop;
    extern ulong *ulip, *ulop;

    switch (pixsize) {
    case (1):			/*char */
	for (x = 0; x < nx; x++) {
	    for (y = ny; y > 0; y--) {
		*ubop++ = *(ubip + y * nx - x - 1);
	    }
	}
	break;
    case (2):			/*ushort */
	for (x = 0; x < nx; x++) {
	    for (y = ny; y > 0; y--) {
		*usop++ = *(usip + y * nx - x - 1);
	    }
	}
	break;
    case (4):			/*ulong */
	for (x = 0; x < nx; x++) {
	    for (y = ny; y > 0; y--) {
		*ulop++ = *(ulip + y * nx - x - 1);
	    }
	}
	break;
    default:
	break;
    }
}


void
transpose1 (int pixsize, int nx, int ny)
{
    /* Transpose the array pointed by *u.ip along
       the main diagonal (/). Output goes to *u.op
     */
    int x, y;
    extern uchar *ubip, *ubop;
    extern ushort *usip, *usop;
    extern ulong *ulip, *ulop;

    switch (pixsize) {
    case (1):			/*char */
	for (x = 0; x < nx; x++) {
	    for (y = 0; y < ny; y++) {
		*ubop++ = *(ubip + y * nx + x);
	    }
	}
	break;
    case (2):			/*ushort */
	for (x = 0; x < nx; x++) {
	    for (y = ny; y > 0; y--) {
		*usop++ = *(usip + y * nx + x);
	    }
	}
	break;
    case (4):			/*ulong */
	for (x = 0; x < nx; x++) {
	    for (y = 0; y < ny; y++) {
		*ulop++ = *(ulip + y * nx + x);
	    }
	}
	break;
    default:
	break;
    }
}
