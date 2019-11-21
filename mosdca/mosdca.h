/*
 * MOSDCA.H -- Global definitions for the Mosaic Data Capture Agent (DCA).
 */

#define	MOSDCA			"mosdca"
#define	DCA_SZIMNAME		64
#define	DCA_SZIMTYPE		64
#define	DCA_MAXDIM		8

/* DCA message tags. */
#define DCA_Quit		1101
#define DCA_SetParam		1102
#define DCA_GetParam		1103
#define DCA_GetStatus		1104
#define DCA_StartReadout	1105
#define DCA_EndReadout		1106
#define DCA_AbortReadout	1107
#define DCA_ConfigureImage	1108
#define DCA_WriteHeaderData	1109
#define DCA_WritePixelData	1110
#define DCA_Synchronize		1111
#define DCA_RequestConsole	1112
#define DCA_ReleaseConsole	1113
#define DCA_SubscribeEvents	1114
#define DCA_UnsubscribeEvents	1115
#define DCA_Event		1116
#define	DCA_WritePixHeader	1117
#define	DCA_WritePixPixels	1118
#define	DCA_TransferComplete	1119

/* Readout stages. */
#define	M_INACTIVE		0
#define	M_START			1
#define	M_READY			2
#define	M_WRITING		3
#define	M_FINISH		4
#define	M_DONE			5
#define	M_ABORT			6


/* Simple image descriptor (DCA_ConfigureImage). */
struct dca_image {
	char name[DCA_SZIMNAME];
	char type[DCA_SZIMTYPE];
	int pixtype;
	int naxes;
	int axlen[DCA_MAXDIM];
	int extend;
};
typedef struct dca_image DCA_Image;

/* Header keyword descriptor (DCA_WriteHeaderData). */
struct dca_keyword {
	char *keyword;
	char *value;
	char *type;
	char *comment;
};
typedef struct dca_keyword DCA_Keyword;

/* Pixel stream descriptor (DCA_WritePixelData). */
struct dca_stream {
	int offset;
	int npix, stride;
	int destimage;
	int x0, y0;
	int xstep, ystep;
	int xydir;
};
typedef struct dca_stream DCA_Stream;
