#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "vmcache.h"
#include "dso.h"
#include "kwdb.h"
#include "dsim.h"

/*
 * DSIM -- Distributed shared image interface.
 *
 *      ptr = dsim_FileName (objectname)
 *         dsim = dsim_Open (vm, objectname, mode)
 *	         dsim_Close (dsim)
 *	     dsim_Configure (dsim)
 *		  dsim_Sync (dsim, async)
 *
 *      image = dsim_Locate (dsim, imagename)
 *	           dsim_Set (dsim, image, param, value)
 *	     val = dsim_Get (dsim, image, param)
 *	        dsim_SetStr (dsim, image, param, value)
 *	  str = dsim_GetStr (dsim, image, param)
 *
 *   kwdb = dsim_ReadHeader (dsim, image, maxentries)
 *         dsim_WriteHeader (dsim, image, kwdb)
 *
 *	  dsim_EncodePixels (dsim, image, in, out, datalen, swapped)
 *	  dsim_DecodePixels (dsim, image, in, out, datalen, swapped)
 *
 *        io = dsim_StartIO (dsim, image, mode, sv, nv)
 *       ptr = dsim_Pixel1D (io, i)
 *       ptr = dsim_Pixel2D (io, i, j)
 *       ptr = dsim_Pixel3D (io, i, j, k)
 *	      dsim_FinishIO (io)
 *
 * Open creates a DSIM object descriptor for the named image and opens or
 * creates the image as indicated by the access mode.  If a new image is
 * being created the file layout will not be known yet and no storage will
 * actually be located, but permissions to create the image will be verified.
 *
 * Locate returns the image index of the named file extension.  Set and Get
 * are used to set or query DSIM parameters.  In particular, Set is used to
 * enter the size and other attributes of a new image.
 *
 * Configure fixes the file attributes and lays out storage for the imagefile.
 * If a new file is being created Configure allocates storage for the file.
 * A configure will be performed automatically by the first i/o operation on
 * any image.
 *
 * ReadHeader creates a KWDB database and reads an image header into it.
 * WriteHeader is used to write or update an image header.  The contents of
 * the keyword database KWDB is copied to the header area of the image.  Any
 * existing header is lost.  If the image contains multiple subimages the
 * "image" argument specifies the header to be written.  image=0 refers to
 * the global image header.
 *
 * StartIO and FinishIO are used to read or write to an image.  StartIO
 * obtains a lock on the image region defined by the vectors SV, NV (starting
 * pixel and number of pixels in each dimension).  FinishIO should be called
 * after access to the region is finished to update the image and release any
 * locks.  Multiple i/o regions can be simultaneously active.  Pixel returns
 * a pointer to the indicated pixel in the region.  The returned pointer
 * should be coerced to the actual pixel type.
 * 
 * This version of the DSIM interface does not implement the full distributed
 * shared image object.  This version provides a similar interface but
 * internally it implements only a mapped image file.  The physical storage
 * format implemented internally is a FITS multi-extension file using the
 * IMAGE extension for image storage.  Headers have a fixed capacity and header
 * keyword attributes have serious limits on string lengths.
 *
 * Although the DSIM interface is image-oriented, the FITS multiextension file
 * produced by DSIM can actually contain extensions other than images.  The
 * image number in the DSIM routines is actually the extension number with
 * zero being the PHU and one being the first extension.  DSIM treats all
 * extensions as images, but a DSIM image is really little more than a file
 * with a general keyword type header and a simple multidimensional data
 * addressing scheme.  To create a non-image extension the "imagetype" field
 * should be set to the FITS extension type.  For a non-image extension the
 * pixtype should be set to byte.  StartIO and Pixel can be used to perform
 * raw i/o to the data (pixel) area of the extension.  The header access
 * routines can be used for any type of extension.
 */

/* #define DEBUG */

#define MAXDIM		8
#define	SZ_NAME		128
#define	SZ_PATHNAME	256
#define	CARDLEN		80
#define	BLKLEN		2880
#define	DEF_MAXKEYWORDS	72		/* default max keywords */
#define	DEF_IMFORMAT	"im%d"		/* image extension name format */
#define	DEF_EXTENSION	".fits"		/* default file extension */
#define	DEF_EXTNLEN	5		/* default file extension */

/* Image descriptor. */
struct image {
	char name[SZ_PATHNAME];		/* image name */
	char type[SZ_PATHNAME];		/* image type */
	int imagenumber;		/* relative image extn number */
	int maxkeywords;		/* max header keywords */
	int nreserved;			/* number of reserved keywords */
	int naxes;			/* number of image axes */
	int axlen[MAXDIM];		/* axis lengths */
	int pixtype;			/* pixel type code */
	int pixsize;			/* pixel size in bytes */
	long extend;			/* nbytes to reserve after image */
	long hdr_offset;		/* file offset to image header */
	long pix_offset;		/* file offset to first pixel */
	long extnsize;			/* total extension size in bytes */
	pointer pixels;			/* memory address of first pixel */
	void (*encode)();		/* pixel encode function */
	void (*decode)();		/* pixel decode function */
};
typedef struct image Image;

/* Region i/o descriptor. */
struct ioregion {
	pointer dsim;			/* backpointer to context DSIM */
	Image *image;			/* image containing i/o region */
	int iomode;			/* access mode */
	int sv[MAXDIM];			/* first pixel of region */
	int nv[MAXDIM];			/* region size */
	int stride[MAXDIM];		/* pixel stride between lines */
	int pixsize;			/* pixel size in bytes */
	pointer pixels;			/* memory address of first pixel */
};
typedef struct ioregion IORegion;

/* DSO imagefile descriptor. */
struct dsimage {
	char name[SZ_PATHNAME];		/* DSO object name (filename) */
	char type[SZ_PATHNAME];		/* DSO object type */
	char imformat[SZ_NAME];		/* image name generation fomat */
	int mode;			/* access mode */
	int fd;				/* file descriptor */
	int configured;			/* set when configure has been called */
	int configuring;		/* set during configure */
	int maxkeywords;		/* max keywords in global header */
	int nreserved;			/* number of reserved keywords */
	int nimages;			/* number of images */
	Image *imagelist;		/* array of image descriptors */
	int nregions;			/* number of regions in iolist */
	IORegion *iolist;		/* i/o region descriptors */
	long hdr_offset;		/* file offset to global header */
	long hdr_length;		/* total extension (PHU) length */
	long filesize;			/* total filesize in bytes */
	void *vm;			/* VM cache descriptor */
	char *mmap_addr;		/* start address of mapped file */
	long mmap_len;			/* size of mapped region in bytes */
};
typedef struct dsimage DSImage;

extern char *getenv();
static char *str();
extern int read(), write();
int ds_read(), ds_write();
static int PixelSize();
static void EncodeUShort(), DecodeUShort();
static void EncodeInt(), DecodeInt();
static int host_swap2();
static void bswap2();

#define	max(a,b)	((a)>(b)?(a):(b))
#define	min(a,b)	((a)>(b)?(b):(a))


/* DSIM_OPEN -- Open a DSO imagefile.  The object descriptor is returned.
 * The access mode determines whether a new or existing imagefile is opened,
 * and whether or not the imagefile is writeable.  Null is returned if the
 * open fails.
 */
pointer
dsim_Open (vm, objectname, mode)
void *vm;
char *objectname;
int mode;
{
	register DSImage *ds;
	char fname[SZ_PATHNAME], *s;
	int nchars, fd;

	/* Just use the objectname as the filename at present, but add the
	 * file extension if it is missing.
	 */
	strcpy (fname, objectname);
	nchars = strlen (fname);
	if (nchars < DEF_EXTNLEN ||
		strcmp (&fname[nchars-DEF_EXTNLEN], DEF_EXTENSION) != 0)
	    strcat (fname, DEF_EXTENSION);

	/* Open the DSO imagefile. */
	switch (mode) {
	case DSO_CREATE:
	    if (access (fname, 0) == 0)
		return (NULL);
	    if ((fd = open (fname, O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0)
		return (NULL);
	    break;
	case DSO_RDONLY:
	    if ((fd = open (fname, O_RDONLY)) < 0)
		return (NULL);
	    break;
	case DSO_RDWR:
	    if ((fd = open (fname, O_RDWR)) < 0)
		return (NULL);
	    break;
	default:
	    return (NULL);
	    break;
	}

	/* Initialize the DSIM descriptor. */
	if (!(ds = (DSImage *) calloc (1, sizeof(DSImage)))) {
	    close (fd);
	    return (NULL);
	}

	ds->vm = vm;
	strcpy (ds->name, objectname);
	ds->mode = mode;
	ds->fd = fd;

	if (mode == DSO_CREATE) {
	    /* Set defaults for a new imagefile.
	     */
	    if (s = getenv("DSIM_MAXKEYWORDS"))
		ds->maxkeywords = atoi(s);
	    else
		ds->maxkeywords = DEF_MAXKEYWORDS;

	    if (s = getenv("DSIM_IMFORMAT"))
		strcpy (ds->imformat, s);
	    else
		strcpy (ds->imformat, DEF_IMFORMAT);

	} else {
	    /* Scan the imagefile and build the image list if opening an
	     * existing imagefile.
	     */
	    free ((char *) ds);
	    close (fd);
	    return (NULL);	/* not yet supported */
	}

	return ((pointer) ds);
}


/* DSIM_CLOSE -- Close an imagefile.
 */
dsim_Close (dsim)
pointer dsim;
{
	register DSImage *ds = (DSImage *)dsim;
	int status = 0;
	extern int errno;

	/* Flush and unmap the imagefile. */
	if (ds->mmap_addr) {
	    if (vm_sync (ds->vm, ds->fd, 0L, ds->mmap_len, 0) < 0) {
		status = -1;
#ifdef DEBUG
		dprintf (0, "dsim", "vm_sync failed, errno=%d\n", errno);
#endif
	    }
	    if (vm_uncacheregion (ds->vm, ds->fd, 0L, ds->mmap_len, 0) < 0) {
		status = -1;
#ifdef DEBUG
		dprintf (0, "dsim", "vm_uncacheregion failed, errno=%d\n",
		    errno);
#endif
	    }
	}

	/* Close the imagefile. */
	if (ds->fd && close (ds->fd) < 0) {
	    status = -1;
#ifdef DEBUG
	    dprintf (0, "dsim", "file close failed, errno=%d\n", errno);
#endif
	}

	/* Free the descriptor. */
	if (ds->iolist)
	    free ((char *)ds->iolist);
	if (ds->imagelist)
	    free ((char *)ds->imagelist);
	free ((char *)ds);

	return (status);
}


/* DSIM_FILENAME -- Get the filename for a DSO imagefile.  A special routine
 * is needed to handle any implied file extensions.
 */
char *
dsim_FileName (objectname)
char *objectname;
{
	static char fname[SZ_PATHNAME];
	int nchars;

	strcpy (fname, objectname);
	nchars = strlen (fname);
	if (nchars < DEF_EXTNLEN ||
		strcmp (&fname[nchars-DEF_EXTNLEN], DEF_EXTENSION) != 0)
	    strcat (fname, DEF_EXTENSION);

	return (fname);
}


/* DSIM_SET -- Set an interface parameter or image attribute (integer
 * parameters only).  image=0 refers to the entire imagefile, image=1-N
 * refers to image 1 through N.  Interface parameters affect the functioning
 * of the interface.  Image attributes determine the characteristics of the
 * image object.  Some attributes refer only to the full imagefile, others
 * only to a single image, others to both.  
 */
dsim_Set (dsim, image, param, value)
pointer dsim;
int image;
int param;
int value;
{
	register DSImage *ds = (DSImage *)dsim;
	register Image *im;
	int axis;

	/* Get pointer to image descriptor. */
	if (!image)
	    im = NULL;
	else if (image < 1 || image > ds->nimages)
	    return (-1);
	else if (!ds->imagelist) {
	    Image *imagelist;
	    if (!(imagelist = (Image *) calloc (ds->nimages, sizeof(Image))))
		return (-1);
	    ds->imagelist = imagelist;
	}
	if (image)
	    im = ds->imagelist + image - 1;

	switch (param) {
	case DSIM_PixelType:
	    if (!im)
		return (-1);
	    im->pixtype = value;
	    break;

	case DSIM_NImages:
	    if (!im)
		ds->nimages = value;
	    else
		return (-1);
	    break;

	case DSIM_NAxes:
	    if (!im)
		return (-1);
	    if (value < 0 || value > MAXDIM)
		return (-1);
	    im->naxes = value;
	    break;

	case DSIM_Axlen1:
	case DSIM_Axlen2:
	case DSIM_Axlen3:
	    if (!im)
		return (-1);
	    axis = param - DSIM_Axlen1;
	    im->axlen[axis] = value;
	    break;

	case DSIM_MaxKeywords:
	    if (!im)
		ds->maxkeywords = value;
	    else
		im->maxkeywords = value;
	    break;

	case DSIM_Extend:
	    if (!im)
		return (-1);
	    else
		im->extend = value;
	    break;

	default:
	    return (-1);
	}

	return (0);
}


/* DSIM_SETSTR -- Set a string valued interface parameter or image attribute.
 */
dsim_SetStr (dsim, image, param, value)
pointer dsim;
int image;
int param;
char *value;
{
	register DSImage *ds = (DSImage *)dsim;
	register Image *im;

	if (!image)
	    im = NULL;
	else if (image < 1 || image > ds->nimages)
	    return (-1);
	else if (!ds->imagelist) {
	    Image *imagelist;
	    if (!(imagelist = (Image *) calloc (ds->nimages, sizeof(Image))))
		return (-1);
	    ds->imagelist = imagelist;
	}
	if (image)
	    im = ds->imagelist + image - 1;

	switch (param) {
	case DSIM_ImageNameFormat:
	    strcpy (ds->imformat, value);
	    break;
	case DSIM_ObjectName:
	    if (!im)
		strcpy (ds->name, value);
	    else
		strcpy (im->name, value);
	    break;
	case DSIM_ObjectType:
	    if (!im)
		strcpy (ds->type, value);
	    else
		strcpy (im->type, value);
	    break;
	default:
	    return (-1);
	}

	return (0);
}


/* DSIM_GET -- Set an interface parameter or image attribute.
 */
dsim_Get (dsim, image, param)
pointer dsim;
int image;
int param;
{
	register DSImage *ds = (DSImage *)dsim;
	register Image *im;
	int axis;

	/* Get pointer to image descriptor. */
	if (!image)
	    im = NULL;
	else if (image < 1 || image > ds->nimages || !ds->imagelist)
	    return (-1);
	else
	    im = ds->imagelist + image - 1;

	switch (param) {
	case DSIM_ImageNumber:
	    if (!im)
		return (-1);
	    return (im->imagenumber);
	    break;

	case DSIM_FileIndex:
	    if (!im)
		return (0);
	    return (image);
	    break;

	case DSIM_NImages:
	    return (ds->nimages);
	    break;

	case DSIM_PixelType:
	    if (!im)
		return (-1);
	    return (im->pixtype);
	    break;

	case DSIM_PixelSize:
	    if (!im)
		return (-1);
	    return (im->pixsize);
	    break;

	case DSIM_PixelConvert:
	    /* Is pixel conversion necessary?
	     */
	    if (!im)
		return (-1);
	    return (im->encode != NULL);
	    break;

	case DSIM_ByteSwapped:
	    /* Are the pixels byte swapped in the stored imagefile?
	     */
	    return (0);
	    break;

	case DSIM_NAxes:
	    if (!im)
		return (-1);
	    return (im->naxes);
	    break;

	case DSIM_Axlen1:
	case DSIM_Axlen2:
	case DSIM_Axlen3:
	    if (!im)
		return (-1);
	    axis = param - DSIM_Axlen1;
	    return (im->axlen[axis]);
	    break;

	case DSIM_MaxKeywords:
	    if (!im)
		return (ds->maxkeywords);
	    else
		return (im->maxkeywords);
	    break;

	case DSIM_Extend:
	    if (!im)
		return (0);
	    else
		return (im->extend);
	    break;

	case DSIM_FileDescriptor:
	    return (ds->fd);
	    break;

	case DSIM_FileAccessMode:
	    return (ds->mode);
	    break;
	}

	return (-1);
}


/* DSIM_GETSTR -- Get a string valued interface parameter or image attribute.
 */
char *
dsim_GetStr (dsim, image, param)
pointer dsim;
int image;
int param;
{
	register DSImage *ds = (DSImage *)dsim;
	register Image *im;

	if (!image)
	    im = NULL;
	else if (image < 1 || image > ds->nimages || !ds->imagelist)
	    return (NULL);
	else
	    im = ds->imagelist + image - 1;

	switch (param) {
	case DSIM_ImageNameFormat:
	    return (ds->imformat);
	    break;
	case DSIM_ObjectName:
	    if (!im)
		return (ds->name);
	    else
		return (im->name);
	    break;
	case DSIM_ObjectType:
	    if (!im)
		return (ds->type);
	    else
		return (im->type);
	    break;
	}

	return (NULL);
}


/* DSIM_LOCATE -- Locate an image given its name.  Returns the image number
 * if successful, zero otherwise.
 */
dsim_Locate (dsim, name)
pointer dsim;
char *name;
{
	register DSImage *ds = (DSImage *)dsim;
	register Image *im;
	register int i;

	if (ds->nimages <= 0 || !ds->imagelist)
	    return (0);

	for (i=0, im = ds->imagelist;  i < ds->nimages;  i++, ds++)
	    if (!strcmp (im->name, name))
		return (i + 1);

	return (0);
}


/* DSIM_READHEADER -- Create a KWDB keyword database and read a header
 * (global datafile or image) into it.  The entire header is read including
 * any reserved keywords.
 */
pointer
dsim_ReadHeader (dsim, image, maxentries)
pointer dsim;
int image;
int maxentries;
{
	register DSImage *ds = (DSImage *)dsim;
	register Image *im;
	pointer kwdb;
	int fd;

	if (!ds->configured && !ds->configuring)
	    if (dsim_Configure (dsim) < 0)
		return (NULL);

	if (!image)
	    im = NULL;
	else if (image < 1 || image > ds->nimages || !ds->imagelist)
	    return (NULL);
	else
	    im = ds->imagelist + image - 1;

	/* Open new, empty KWDB. */
	if (!(kwdb = kwdb_Open (im ? im->name : ds->name)))
	    return (NULL);

	/* Scan the file into the KWDB. */
	if (fd = ds_open (dsim, O_RDONLY) < 0)
	    goto abort;
	if (ds_lseek (fd, im ? im->hdr_offset : 0L, SEEK_SET) < 0)
	    goto abort;

	kwdb_SetIO (kwdb, &ds_read, &ds_write);
	if (kwdb_ReadFITS (kwdb, fd, maxentries, NULL) < 0) {
	    ds_close (fd);
abort:	    kwdb_Close (kwdb);
	    return (NULL);
	}

	kwdb_SetIO (kwdb, &read, &write);
	ds_close (fd);

	return (kwdb);
}


/* DSIM_WRITEHEADER -- Copy a KWDB keyword database into a header (global
 * datafile or image).  Any existing header is overwritten in the process
 * except for the mandatory reserved keywords at the beginning of each header,
 * which are controlled by DSIM (copies of the reserved keywords in the KWDB
 * are ignored and are not copied to the header).  There is no provision for
 * extending the file if the header overflows: the maxkeywords attribute can
 * be set at image creation time to avoid header overflow.
 *
 * The reserved keywords controlled by DSIM are as follows.  These keywords
 * are written to the imagefile when it is configured (dsim_Configure).
 *
 *    PHU:
 *            SIMPLE  = T
 *            BITPIX  = 8
 *            NAXIS   = 0
 *            EXTEND  = T
 *            NEXTEND = <n>
 *            FILENAME= '<name>  '
 *                [KWDB keywords]
 *            END
 *    
 *    EHU:
 *            XTENSION= 'IMAGE   '
 *            BITPIX  = 16
 *            NAXIS   = 2
 *            NAXIS1  = <n>
 *            NAXIS2  = <n>
 *            PCOUNT  = 0
 *            GCOUNT  = 1
 *            EXTNAME = '<name>  '
 *                [KWDB keywords]
 *            END
 *
 * The contents of the KWDB are written to the header in the space indicated,
 * excluding any of the reserved file formatting keywords shown.  For the PHU
 * NEXTEND corresponds to DSIM_NImages.  For an extension header (EHU),
 * XTENSION is given by the DSIM_ObjectType parameter for the image, which
 * defaults to IMAGE.  The extension name is given by DSIM_ObjectName.  The
 * DSIM_PixelType, DSIM_NAxes, and DSIM_Axlen parameters are used to generate
 * values for BITPIX, NAXIS, and NAXISn.  GCOUNT is always 1.  PCOUNT is
 * nonzero, with space reserved in the file, if a value is entered for
 * DSIM_Extend.
 */
dsim_WriteHeader (dsim, image, kwdb)
pointer dsim;
int image;
pointer kwdb;
{
	register Image *im;
	register DSImage *ds = (DSImage *)dsim;
	int ncards, maxcards, fd = -1, ep, xp, i;
	int nreserved, status = -1;
	char card[100];
	pointer rskw=NULL;
	long offset;

	if (!ds->configured && !ds->configuring)
	    if (dsim_Configure (dsim) < 0)
		goto done;

	if (!image) {
	    im = NULL;
	    maxcards = ds->maxkeywords;
	} else {
	    if (image < 1 || image > ds->nimages || !ds->imagelist)
		goto done;
	    im = ds->imagelist + image - 1;
	    maxcards = im->maxkeywords;
	}

	/* Read in the reserved header keywords. */
	nreserved = image ? im->nreserved : ds->nreserved;
	if (!(rskw = dsim_ReadHeader (dsim, image, nreserved)))
	    goto done;

	/* Delete any copies of the reserved keywords from the KWDB.  */
	for (ep=kwdb_Head(rskw);  ep;  ep=kwdb_Next(rskw,ep))
	    if (xp = kwdb_Lookup (kwdb, kwdb_KWName(rskw,ep), 0))
		kwdb_DeleteEntry (kwdb, xp);
	if (xp = kwdb_Lookup (kwdb, "END", 0))
	    kwdb_DeleteEntry (kwdb, xp);

	/* Skip over the reserved keyword area. */
	if (fd = ds_open (dsim, O_RDWR) < 0)
	    goto done;
	offset = (im ? im->hdr_offset : ds->hdr_offset) + nreserved * CARDLEN;
	if (ds_lseek (fd, offset, SEEK_SET) < 0)
	    goto done;

	/* Output the KWDB as a FITS file header.  */
	if ((nreserved + kwdb_Len(kwdb) + 1) > maxcards)
	    goto done;
	kwdb_SetIO (kwdb, &ds_read, &ds_write);
	if ((ncards = kwdb_WriteFITS (kwdb, fd)) < 0)
	    goto done;

	/* Blank fill the remainder of the header area. */
	memset (card, ' ', CARDLEN);
	for (i = nreserved + ncards + 1;  i < maxcards;  i++)
	    if (ds_write (fd, card, CARDLEN) != CARDLEN)
		return (-1);

	/* Write the END card to mark the end of the header.  */
	strcpy (card, "END");
	memset (card+3, ' ', CARDLEN-3);
	if (ds_write (fd, card, CARDLEN) != CARDLEN)
	    return (-1);

	status = 0;
done:
	if (fd >= 0)
	    ds_close (fd);
	if (rskw)
	    kwdb_Close (rskw);
	kwdb_SetIO (kwdb, &read, &write);

	return (status);
}


/* DSIM_ENCODEPIXELS -- Encode a pixel array (if necessary) for storage in
 * the imagefile.  This is a no-op if encoding is not needed for the
 * particular image, file format, and pixel type.
 */
dsim_EncodePixels (dsim, image, in, out, datalen, swapped)
pointer dsim;
int image;
pointer in, out;		/* input and output pixel arrays */
int datalen;			/* data length in bytes (not pixels) */
int swapped;			/* input pixels are byte swapped */
{
	register DSImage *ds = (DSImage *)dsim;
	register Image *im;

	if (!ds->configured)
	    if (dsim_Configure (dsim) < 0)
		return (-1);

	if (image < 1 || image > ds->nimages || !ds->imagelist)
	    return (-1);
	else
	    im = ds->imagelist + image - 1;

	if (im->encode)
	    (im->encode) (in, out, datalen, swapped);

	return (0);
}


/* DSIM_DECODEPIXELS -- Decode a pixel array (if necessary) to restore it to
 * the native host binary format from the format used in the imagefile.
 * This is a no-op if encoding is not needed for the particular image, file
 * format, and pixel type.
 */
dsim_DecodePixels (dsim, image, in, out, datalen, swapped)
pointer dsim;
int image;
pointer in, out;		/* input and output pixel arrays */
int datalen;			/* data length in bytes (not pixels) */
int swapped;			/* output pixels are byte swapped */
{
	register DSImage *ds = (DSImage *)dsim;
	register Image *im;

	if (!ds->configured)
	    if (dsim_Configure (dsim) < 0)
		return (-1);

	if (image < 1 || image > ds->nimages || !ds->imagelist)
	    return (-1);
	else
	    im = ds->imagelist + image - 1;

	if (im->decode)
	    (im->decode) (in, out, datalen, swapped);

	return (0);
}


/* DSIM_STARTIO -- Return a region of the image to the caller to read or
 * modify.  The mode argument specifies the type of operation.  DSIM may
 * lock the region if necessary.  The region to be accessed is specified by
 * the image number and destination rect parameters SV,NV.
 */
pointer
dsim_StartIO (dsim, image, mode, sv, nv)
pointer dsim;
int image;
int mode;
int *sv, *nv;
{
	register DSImage *ds = (DSImage *)dsim;
	register IORegion *rp;
	register Image *im;
	int naxes, pixoff, i;

	if (!ds->configured)
	    if (dsim_Configure (dsim) < 0)
		return (NULL);

	if (image < 1 || image > ds->nimages || !ds->imagelist)
	    return (NULL);
	else
	    im = ds->imagelist + image - 1;

	/* Verify that the requested i/o region is inbounds. */
	for (i=0, naxes = im->naxes;  i < naxes;  i++)
	    if (sv[i] < 0 || (sv[i]+nv[i]) > im->axlen[i])
		return (NULL);

	/* Look for an existing unused region descriptor. */
	for (rp=ds->iolist;  rp && rp < ds->iolist+ds->nregions;  rp++)
	    if (!rp->image)
		break;

	/* If there are no unused region descriptors get more descriptors.
	 */
	if (!rp || rp >= ds->iolist + ds->nregions) {
	    int i, nbytes, nregions;
	    nregions = max(4, ds->nregions * 2);
	    nbytes = nregions * sizeof(IORegion);

	    if (!ds->iolist && !(ds->iolist = (IORegion *) malloc (nbytes)))
		return (NULL);
	    if (!(ds->iolist = (IORegion *) realloc (ds->iolist, nbytes)))
		return (NULL);

	    for (i=ds->nregions;  i < nregions;  i++)
		(ds->iolist + i)->image = NULL;

	    rp = ds->iolist + ds->nregions;
	    ds->nregions = nregions;
	}

	/* Set up the i/o region descriptor.
	 */
	rp->dsim = dsim;
	rp->image = im;
	rp->iomode = mode;

	for (i=0;  i < MAXDIM;  i++) {
	    rp->sv[i] = (i < naxes) ? sv[i] : 1;
	    rp->nv[i] = (i < naxes) ? nv[i] : 1;
	    rp->stride[i] = i ? (rp->stride[i-1] * im->axlen[i-1]) : 1;
	}

	rp->pixsize = im->pixsize;
	rp->pixels = im->pixels;

	return ((pointer) rp);
}


/* DSIM_FINISHIO -- Inform the DSO that i/o on the given region is finished
 * so that the region can be released.
 */
dsim_FinishIO (io)
pointer io;
{
	register IORegion *rp = (IORegion *)io;
	rp->image = NULL;
	return (0);
}


/* DSIM_PIXEL1D -- Get a pointer to the given pixel in a 1D i/o region.
 * The pixel coordinates are those of the full image, not the i/o region,
 * but the pixel and any referenced pixels must be in the region.
 */
pointer
dsim_Pixel1D (io, i)
pointer io;
register int i;
{
	register IORegion *rp = (IORegion *)io;
	return ((pointer) ((char *)rp->pixels + i * rp->pixsize));
}


/* DSIM_PIXEL2D -- Get a pointer to the given pixel in a 2D i/o region.
 * The pixel coordinates are those of the full image, not the i/o region,
 * but the pixel and any referenced pixels must be in the region.
 */
pointer
dsim_Pixel2D (io, i, j)
pointer io;
register int i, j;
{
	register IORegion *rp = (IORegion *)io;
	return ((pointer) ((char *)rp->pixels +
	    (j * rp->stride[1] + i) * rp->pixsize));
}


/* DSIM_PIXEL3D -- Get a pointer to the given pixel in a 3D i/o region.
 * The pixel coordinates are those of the full image, not the i/o region,
 * but the pixel and any referenced pixels must be in the region.
 */
pointer
dsim_Pixel3D (io, i, j, k)
pointer io;
register int i, j, k;
{
	register IORegion *rp = (IORegion *)io;
	return ((pointer) ((char *)rp->pixels +
	    (k * rp->stride[2] + j * rp->stride[1] + i) * rp->pixsize));
}


/* DSIM_CONFIGURE -- Fix the geometry (file layout) of the imagefile.  Should
 * not be called until the primary parameters of each image (or other
 * extension) have been set.  These include the number of images, the size
 * and pixel type of each image, and the max global and image header sizes.
 */
dsim_Configure (dsim)
pointer dsim;
{
	register Image *im;
	register DSImage *ds = (DSImage *)dsim;
	int pixblocks, bytes, image, acmode, nim=0, npix, i, status = -1;
	char buf[SZ_NAME];
	pointer kwdb;
	void *addr;

	if (ds->configured)
	    return (0);
	else
	    ds->configuring++;

	if (ds->mode != DSO_CREATE)
	    goto mapfile;

	/* We are configuring a new file.  Compute the geometry of the
	 * global file header.
	 */
	if (!ds->maxkeywords)
	    ds->maxkeywords = DEF_MAXKEYWORDS;

	/* Round up to an integral number of FITS logical blocks. */
	bytes = ds->maxkeywords * CARDLEN;
	bytes = (bytes + BLKLEN - 1) / BLKLEN * BLKLEN;
	ds->maxkeywords = bytes / CARDLEN;

	ds->hdr_offset = 0;
	ds->hdr_length = ds->maxkeywords * CARDLEN;
	ds->filesize = ds->hdr_length;

	if (!ds->nimages)
	    return (0);

	/* Write out the global file header.
	 */
	if (!(kwdb = kwdb_Open ("PHU")))
	    goto done;

	kwdb_AddEntry (kwdb, "SIMPLE",   "T", "L",
	    "File conforms to FITS standard");
	kwdb_AddEntry (kwdb, "BITPIX",   "8", "N",
	    "Bits per pixel (not used)");
	kwdb_AddEntry (kwdb, "NAXIS",    "0", "N",
	    "PHU contains no image matrix");
	kwdb_AddEntry (kwdb, "EXTEND",   "T", "L",
	    "File contains extensions");
	kwdb_AddEntry (kwdb, "NEXTEND",  str(ds->nimages), "N",
	    "Number of extensions");
	kwdb_AddEntry (kwdb, "FILENAME", ds->name, "S",
	    "Original host filename");

	if (dsim_WriteHeader (dsim, 0, kwdb) < 0)
	    goto done;
	ds->nreserved = kwdb_Len (kwdb);
	kwdb_Close (kwdb);
	kwdb = NULL;

	/* Compute the geometry of each image and write out the reserved
	 * header keywords.
	 */
	for (image=1,im=ds->imagelist;  image <= ds->nimages;  image++, im++) {
	    /* Set the extension type, default IMAGE if none given. */
	    if (!im->type[0])
		strcpy (im->type, "IMAGE");

	    /* Set the extension name.  IMAGE extensions are numbered
	     * separately from other extensions using imformat to generate
	     * the extension name.  Other extensions are merely named
	     * "extnXX" where XX is the absolute extension number.
	     */
	    sprintf (buf, "extn%d", image);
	    if (!strcmp (im->type, "IMAGE")) {
		sprintf (buf, ds->imformat, ++nim);
		im->imagenumber = nim;
	    }
	    if (!im->name[0])
		strcpy (im->name, buf);

	    if (!im->maxkeywords)
		im->maxkeywords = ds->maxkeywords;

	    /* Round up to an integral number of FITS logical blocks. */
	    bytes = im->maxkeywords * CARDLEN;
	    bytes = (bytes + BLKLEN - 1) / BLKLEN * BLKLEN;
	    im->maxkeywords = bytes / CARDLEN;

	    im->hdr_offset = ds->filesize;
	    im->pix_offset = im->hdr_offset + im->maxkeywords * CARDLEN;
	    ds->filesize = im->pix_offset;

	    if (!im->naxes)
		; /* probably an error, but technically permissible. */

	    /* Set the length of the degenerate axes to 1. */
	    for (i=im->naxes;  i < MAXDIM;  i++)
		im->axlen[i] = 1;

	    if (im->naxes && !im->pixtype)
		goto done;	/* definitely an error */
	    else
		im->pixsize = PixelSize (im->pixtype);

	    /* Compute the image geometry.
	     */
	    npix = im->naxes ? im->axlen[0] : 0;
	    for (i=1;  i < im->naxes;  i++)
		npix *= im->axlen[i];

	    pixblocks = ((npix * im->pixsize) + BLKLEN-1) / BLKLEN;
	    ds->filesize += pixblocks * BLKLEN;

	    /* Round up the extend area if any. */
	    im->extend = (im->extend + BLKLEN-1) / BLKLEN * BLKLEN;
	    ds->filesize += im->extend;

	    /* Compute the full size in bytes of the extension. */
	    im->extnsize = ds->filesize - im->hdr_offset;

	    /* Write the reserved header keywords to the EHU. */
	    if (!(kwdb = kwdb_Open ("EHU")))
		goto done;

	    kwdb_AddEntry (kwdb, "XTENSION", im->type, "S",
		"Extension type");
	    kwdb_AddEntry (kwdb, "BITPIX",   str(im->pixsize*8), "N",
		"Bits per pixel");

	    kwdb_AddEntry (kwdb, "NAXIS",    str(im->naxes), "N",
		"Number of image axes");
	    for (i=1;  i <= im->naxes;  i++) {
		char com[64];
		sprintf (buf, "NAXIS%d", i);
		sprintf (com, "Length of axis %d", i);
		kwdb_AddEntry (kwdb, buf,    str(im->axlen[i-1]), "N", com);
	    }

	    /* Undo -32768 shift to restore unsigned short value from FITS. */
	    if (im->pixtype == DSO_USHORT) {
		kwdb_AddEntry (kwdb, "BSCALE", "1", "N",
		    "Scale factor for unsigned short");
		kwdb_AddEntry (kwdb, "BZERO", "32768", "N",
		    "Zero shift for unsigned short");
	    }

	    kwdb_AddEntry (kwdb, "PCOUNT",   str(im->extend), "N",
		"Number of bytes following image matrix");
	    kwdb_AddEntry (kwdb, "GCOUNT",   str(1), "N",
		"Number of groups"); 
	    kwdb_AddEntry (kwdb, "EXTNAME",  im->name, "S",
		"Extension name");

	    if (dsim_WriteHeader (dsim, image, kwdb) < 0)
		goto done;
	    im->nreserved = kwdb_Len (kwdb);
	    kwdb_Close (kwdb);
	    kwdb = NULL;

	    /* Is special pixel conversion needed to store pixels in the
	     * imagefile?
	     */
	    if (im->pixtype == DSO_INT) {
		im->encode = EncodeInt;
		im->decode = NULL;
	    } else if (im->pixtype == DSO_USHORT) {
		im->encode = EncodeUShort;
		im->decode = DecodeUShort;
	    } else {
		im->encode = NULL;
		im->decode = NULL;
	    }
	}

	/* Set the physical file size. */
	buf[0] = '\0';
	if (lseek (ds->fd, ds->filesize-1, SEEK_SET) < 0)
	    goto done;
	if (write (ds->fd, buf, 1) != 1)
	    goto done;
mapfile:
	/* Attempt to map the file into memory. */
	acmode = (ds->mode==DSO_RDONLY) ? VM_READONLY : VM_READWRITE;
	addr = vm_cacheregion (ds->vm, ds->fd, 0L, ds->filesize, acmode, 0);

#ifdef DEBUG
	dprintf (0, "dsim", "image %s mapped into vmcache at 0x%x, size %d\n",
	    ds->name, addr, ds->filesize);
#endif

	if (addr == (void *) -1)
	    goto done;

	ds->mmap_addr = (char *)addr;
	ds->mmap_len = ds->filesize;

	for (image=1;  image <= ds->nimages;  image++) {
	    im = ds->imagelist + image - 1;
	    im->pixels = (pointer) ((char *)addr + im->pix_offset);
	}

	ds->configured++;
	ds->configuring = 0;
	status = 0;
done:
	if (kwdb)
	    kwdb_Close (kwdb);

	return (status);
}


/* DSIM_SYNC -- Flush any buffered imagefile data to disk.
 */
dsim_Sync (dsim, async)
pointer dsim;
int async;
{
	register DSImage *ds = (DSImage *)dsim;
	int status = 0;

	if (ds->mmap_addr)
	    status = vm_sync (ds->vm, ds->fd, 0L, ds->mmap_len,
		async ? VM_ASYNC : 0);

#ifdef DEBUG
	dprintf (0, "dsim", "vm_sync at 0x%x size %d async=%d, status=%d\n",
	    ds->mmap_addr, ds->mmap_len, async, status);
#endif

	return (status);
}


/*
 * Internal routines.
 * ------------------
 */

/*
 * DS_FIO -- A small open/close/read/write Unix file emulator used to allow
 * simulated file i/o on mapped files.
 *
 *	 fd = ds_open (dsim, mode)
 *	     ds_close (fd)
 *	      ds_read (fd, buf, maxbytes)
 *	     ds_write (fd, buf, nbytes)
 *	     ds_lseek (fd, offset, whence)
 *
 * Offset is the byte offset in the imagefile mapped by DSIM.  If the DSIM
 * imagefile has not been configured and mapped yet then ordinary file i/o
 * is performed on the real imagefile.
 */

#define MAX_FD 32
struct dsim_fio {
	pointer dsim;
	int mode;
	long fpos;
};
static struct dsim_fio fdtable[MAX_FD];

ds_open (dsim, mode)
pointer dsim;
int mode;
{
	register int i;

	for (i=0;  i < MAX_FD;  i++)
	    if (!fdtable[i].dsim) {
		fdtable[i].dsim = dsim;
		fdtable[i].mode = mode;
		fdtable[i].fpos = 0;
		return (i);
	    }

	return (-1);
}

ds_close (fd)
int fd;
{
	if (fd < 0 || fd >= MAX_FD)
	    return (-1);
	fdtable[fd].dsim = NULL;
	return (0);
}

ds_read (fd, buf, maxbytes)
int fd;
char *buf;
int maxbytes;
{
	register struct dsim_fio *fp;
	register DSImage *ds;
	int nbytes;

	if (fd < 0 || fd >= MAX_FD)
	    return (-1);
	else
	    fp = &fdtable[fd];

	ds = (DSImage *)fp->dsim;
	if (!ds->configured || !ds->mmap_addr) {
	    int n = read (ds->fd, buf, maxbytes);
	    return (n);
	}

	if (fp->fpos < 0 || fp->fpos > ds->filesize)
	    return (-1);

	nbytes = min (ds->filesize, fp->fpos + maxbytes) - fp->fpos;
	memcpy (buf, ds->mmap_addr + fp->fpos, nbytes);
	fp->fpos += nbytes;

	return (nbytes);
}

ds_write (fd, buf, maxbytes)
int fd;
char *buf;
int maxbytes;
{
	register struct dsim_fio *fp;
	register DSImage *ds;
	int nbytes;

	if (fd < 0 || fd >= MAX_FD)
	    return (-1);
	else
	    fp = &fdtable[fd];

	ds = (DSImage *)fp->dsim;
	if (!ds->configured || !ds->mmap_addr) {
	    int n = write (ds->fd, buf, maxbytes);
	    return (n);
	}

	if (fp->fpos < 0 || fp->fpos > ds->filesize)
	    return (-1);

	nbytes = min (ds->filesize, fp->fpos + maxbytes) - fp->fpos;
	memcpy (ds->mmap_addr + fp->fpos, buf, nbytes);
	fp->fpos += nbytes;

	return (nbytes);
}

ds_lseek (fd, offset, whence)
int fd;
long offset;
int whence;
{
	register struct dsim_fio *fp;
	register DSImage *ds;

	if (fd < 0 || fd >= MAX_FD || whence != SEEK_SET)
	    return (-1);
	else
	    fp = &fdtable[fd];

	ds = (DSImage *)fp->dsim;
	if (!ds->configured || !ds->mmap_addr)
	    return (lseek (ds->fd, offset, whence));
	else
	    return (fp->fpos = offset);
}


/* STR -- Simple routine to convert an integer to a decimal string.
 */
static char *
str (n)
int n;
{
	static char s[32];
	sprintf (s, "%d", n);
	return (s);
}


/* PixelSize -- Return the size in bytes of a data element.
 */
static int
PixelSize (type)
int type;
{
	switch (type) {
	case DSO_UBYTE:
	case DSO_CHAR:
	    return (sizeof(char));
	case DSO_SHORT:
	case DSO_USHORT:
	    return (sizeof(short));
	case DSO_INT:
	    return (sizeof(int));
	case DSO_LONG:
	    return (sizeof(long));
	case DSO_REAL:
	    return (sizeof(float));
	case DSO_DOUBLE:
	    return (sizeof(double));
	default:
	    return (0);
	}
}


/* EncodeUShort -- Encode a unsigned short pixel for storage in the imagefile.
 * FITS can't store ushort directly, one must shift the value to the range of
 * a signed short and set BSCALE,BZERO in the image header.  It is up to the
 * header code to handle the BSCALE,BZERO, all we need to do here is translate
 * to the range of a signed short.
 */
static void
EncodeUShort (pixels_in, pixels_out, datalen, swapped)
pointer pixels_in, pixels_out;
int datalen, swapped;
{
	register unsigned short *in = (unsigned short *) pixels_in;
	register short *out = (short *) pixels_out;
	register int n = datalen / sizeof(short);

	/* Swap to host order if necessary. */
	if (swapped != host_swap2()) {
	    bswap2 (pixels_in, pixels_out, datalen);
	    in = (unsigned short *) pixels_out;
	}

	/* Apply the BZERO offset. */
	while (--n >= 0)
	    *out++ = (short) (((int) *in++) - 32768);

	/* Swap to FITS order if necessary. */
	if (host_swap2())
	    bswap2 (pixels_out, pixels_out, datalen);
}


/* DecodeUShort -- Decode an encoded unsigned short pixel array.
 */
static void
DecodeUShort (pixels_in, pixels_out, datalen, swapped)
pointer pixels_in, pixels_out;
int datalen, swapped;
{
	register short *in = (short *) pixels_in;
	register unsigned short *out = (unsigned short *) pixels_out;
	register int n = datalen / sizeof(short);

	/* Swap from FITS order if necessary. */
	if (host_swap2()) {
	    bswap2 (pixels_in, pixels_out, datalen);
	    in = (short *) pixels_out;
	}

	/* Remove the BZERO offset. */
	while (--n >= 0)
	    *out++ = (unsigned short) (((int) *in++) + 32768);

	/* Swap to the output byte order if necessary. */
	if (swapped != host_swap2())
	    bswap2 (pixels_out, pixels_out, datalen);
}

/* EncodeInt -- Encode a int pixel for storage in the imagefile.
 */
static void
EncodeInt (pixels_in, pixels_out, datalen, swapped)
pointer pixels_in, pixels_out;
int datalen, swapped;
{
	register unsigned int *in = (unsigned int *) pixels_in;
	register int *out = (int *) pixels_out;
	register int i, n = datalen / sizeof(int);
	register unsigned char *ptr = pixels_in;

            for (i = 0; i < n; i++, ptr += 4) {
                *(unsigned int *)out = (((unsigned int)*ptr) << 24) | 
                    ((unsigned int)ptr[1] << 16) | 
                    ((unsigned int)ptr[2] << 8)  | 
                    (unsigned int)ptr[3];
		out++;
	    }
	    return;
}


/* DecodeInt -- Decode an encoded int pixel array.
 */
static void
DecodeInt (pixels_in, pixels_out, datalen, swapped)
pointer pixels_in, pixels_out;
int datalen, swapped;
{
}


/* HOST_SWAP2 -- Test whether the current host computer stores 2-byte short
 * integers with the bytes swapped (little-endian).
 */
static int
host_swap2()
{
	static unsigned short val = 1;
	char *byte = (char *) &val;
	return (byte[0] != 0);
}


/* HOST_SWAP4 -- Test whether the current host computer stores 2-byte short
 * integers with the bytes swapped (little-endian).
 */
static int
host_swap4()
{
	static unsigned int val = 1;
	char *byte = (char *) &val;
	return (byte[0] != 0);
}


/* BSWAP2 - Move bytes from array "a" to array "b", swapping successive
 * pairs of bytes.  The two arrays may be the same but may not be offset
 * and overlapping.
 */
static void
bswap2 (a, b, nbytes)
pointer	a;			/* input array */
pointer	b;			/* output array */
int	nbytes;			/* number of bytes to swap */
{
	register char *ip, *op, *otop;
	register unsigned temp;

	ip = (char *)a;
	op = (char *)b;
	otop = op + (nbytes & ~1);

	/* Swap successive pairs of bytes.
	 */
	while (op < otop) {
	    temp  = *ip++;
	    *op++ = *ip++;
	    *op++ = temp;
	}

	/* If there is an odd byte left, move it to the output array.
	 */
	if (nbytes & 1)
	    *op = *ip;
}
