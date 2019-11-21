/*
 * DSIM.H -- Public definitions for the DSIM (Distributed Shared Image)
 *interface.
 */

/* Interface or object parameters. */
#define DSIM_Axlen1		1
#define DSIM_Axlen2		2
#define DSIM_Axlen3		3
#define DSIM_Extend		4
#define DSIM_FileAccessMode	5
#define DSIM_FileDescriptor	6
#define DSIM_FileIndex		7
#define DSIM_ImageNameFormat	8
#define DSIM_ImageNumber	9
#define DSIM_MaxKeywords	10
#define DSIM_NAxes		11
#define DSIM_NImages		12
#define DSIM_ObjectName		13
#define DSIM_ObjectType		14
#define DSIM_PixelSize		15
#define DSIM_PixelType		16
#define DSIM_PixelConvert	17
#define DSIM_ByteSwapped	18

/* Public functions. */
char *dsim_FileName (/* objectname */);
int dsim_Close (/* dsim */);
int dsim_Configure (/* dsim */);
int dsim_FinishIO (/* io */);
int dsim_Get (/* dsim, image, param */);
char *dsim_GetStr (/* dsim, image, param */);
int dsim_Locate (/* dsim, name */);
pointer dsim_Open (/* objectname, mode */);
pointer dsim_Pixel (/* dsim, io, i, j */);
pointer dsim_Pixel1D (/* io, i */);
pointer dsim_Pixel2D (/* io, i, j */);
pointer dsim_Pixel3D (/* io, i, j, k */);
pointer dsim_ReadHeader (/* dsim, image, maxentries */);
int dsim_Set (/* dsim, image, param, value */);
int dsim_SetStr (/* dsim, image, param, value */);
pointer dsim_StartIO (/* dsim, image, mode, sv, nv */);
int dsim_WriteHeader (/* dsim, image, kwdb */);
