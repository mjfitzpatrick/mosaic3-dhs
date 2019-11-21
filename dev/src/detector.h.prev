/**
 *  DETECTOR.H -- Detector definitions for Mosaic 1.1
 */

/*****************************************************************************/
/**  Detector Layout                                                        **/
/*****************************************************************************/


/**
 *  DO NOT DELETE		
 */
typedef struct {
    char  *colID;			/* collector ID			*/
    int    pageNum;			/* shared memory page number	*/
    int    destNum;			/* image destination extn 	*/
} pixArray, *pixArrayP;

/*****************************************************************************/


#define CCD_W		  2148		/**  width		       **/
#define CCD_H		  4096		/**  height		       **/
#define CCD_BPP		  4		/**  bytes per pixel	       **/
#define CCD_ROWS	  32		/**  No rows per chunk         **/
#define NCCDS		  8		/**  No of CCDs		       **/
#define NAMPS		  2		/**  No of amplifiers	       **/

#define BIAS_WIDTH        50
#define REFERENCE_WIDTH   BIAS_WIDTH
#define BITPIX            32


#ifndef _RASINFO_DEFINED

#ifndef SPLIT_AMPS

static pixArray rasInfo[] = {
    { "PanA",	1,	4},		/*  +-------------------------+	*/
    { "PanA",	2,	3},		/*  |      |     |      |     | */
    { "PanA",	3,	2},		/*  |  A5  | A6  |  A7  |  A8 | */
    { "PanA",	4,	1},		/*  +-------------------------+	*/
    { "PanA",	5,	5},		/*  |      |     |      |     | */
    { "PanA",	6,	6},		/*  |  A4  | A3  |  A2  |  A1 | */
    { "PanA",	7,	7},		/*  +-------------------------+	*/
    { "PanA",	8,	8}
};

#else

#ifdef OLD_WRONG
static pixArray rasInfo[] = {
    { "PanA",	1,	7},		/*  +-------------------------+	*/
    { "PanA",	2,	5},		/*  |      |     |      |     | */
    { "PanA",	3,	3},		/*  |  A5  | A6  |  A7  |  A8 | */
    { "PanA",	4,	1},		/*  +-------------------------+	*/
    { "PanA",	5,	9},		/*  |      |     |      |     | */
    { "PanA",	6,	11},		/*  |  A4  | A3  |  A2  |  A1 | */
    { "PanA",	7,	13},		/*  +-------------------------+	*/
    { "PanA",	8,	15}
};

#else

static pixArray rasInfo[] = {
    { "PanA",	1,	4},		/*  +-------------------------+	*/
    { "PanA",	2,	3},		/*  |      |     |      |     | */
    { "PanA",	3,	2},		/*  |  A5  | A6  |  A7  |  A8 | */
    { "PanA",	4,	1},		/*  +-------------------------+	*/
    { "PanA",	5,	8},		/*  |      |     |      |     | */
    { "PanA",	6,	7},		/*  |  A4  | A3  |  A2  |  A1 | */
    { "PanA",	7,	6},		/*  +-------------------------+	*/
    { "PanA",	8,	5}
};
#endif

#endif

#define _RASINFO_DEFINED
#endif
