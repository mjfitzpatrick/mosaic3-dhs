/**
 *  PXFDCA.H -- Global definitions for the PXF Data Capture Agent client.
 */

#include "location.h"
#include "instrument.h"
#include "detector.h"

#define NDCA                  "ndca"
#define DCA_SZIMNAME            64
#define DCA_SZIMTYPE            64
#define DCA_MAXDIM              8

/*  DCA message tags
 */
#define DCA_Quit                1101
#define DCA_SetParam            1102
#define DCA_GetParam            1103
#define DCA_GetStatus           1104
#define DCA_StartReadout        1105
#define DCA_EndReadout          1106
#define DCA_AbortReadout        1107
#define DCA_ConfigureImage      1108
#define DCA_WriteHeaderData     1109
#define DCA_WritePixelData      1110
#define DCA_Synchronize         1111
#define DCA_RequestConsole      1112
#define DCA_ReleaseConsole      1113
#define DCA_SubscribeEvents     1114
#define DCA_UnsubscribeEvents   1115
#define DCA_Event               1116
#define DCA_WritePixHeader      1117
#define DCA_WritePixPixels      1118

/*  Readout stages
 */
#define M_INACTIVE              0
#define M_START                 1
#define M_READY                 2
#define M_WRITING               3
#define M_FINISH                4
#define M_DONE                  5
#define M_ABORT                 6


/*  Simple image descriptor (DCA_ConfigureImage)
 */
struct dca_image {
        char name[DCA_SZIMNAME];
        char type[DCA_SZIMTYPE];
        int pixtype;
        int naxes;
        int axlen[DCA_MAXDIM];
        int extend;
};
typedef struct dca_image DCA_Image;

/* Header keyword descriptor (DCA_WriteHeaderData)
 */
struct dca_keyword {
        char *keyword;
        char *value;
        char *type;
        char *comment;
};
typedef struct dca_keyword DCA_Keyword;

/* Pixel stream descriptor (DCA_WritePixelData)
 */
struct dca_stream {
        int offset;
        int npix, stride;
        int destimage;
        int x0, y0;
        int xstep, ystep;
        int xydir;
};
typedef struct dca_stream DCA_Stream;


/**
 *  Public definitions for the DSO interfaces.
 */

/* Types
 */
typedef void *pointer;

#ifndef OK
#define OK 0
#endif
#ifndef ERR
#define ERR (-1)
#endif
#ifndef EOS
#define EOS '\0'
#endif

/* Access modes
 */
#define DSO_RDONLY		0
#define DSO_RDWR		1
#define DSO_CREATE		2

/* Data types
 */
#define	DSO_UBYTE		1
#define	DSO_CHAR		2
#define	DSO_SHORT		3
#define	DSO_USHORT		4
#define	DSO_INT			5
#define	DSO_LONG		6
#define	DSO_REAL		7
#define	DSO_DOUBLE		8

/* Generic message tags
 */
#define MSG_Quit		1001
#define MSG_SetParam		1002
#define MSG_GetParam		1003
#define MSG_GetStatus		1004

/* Compatibility garbage
 */
#ifndef SEEK_SET
#define SEEK_SET 		0
#endif


