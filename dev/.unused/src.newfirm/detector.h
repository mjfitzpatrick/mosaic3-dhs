/**
 *  DETECTOR.H -- Detector definitions for NEWFIRM
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


#define CCD_W		  2112		/**  width		       **/
#define CCD_H		  2048		/**  height		       **/
#define CCD_BPP		  4		/**  bytes per pixel	       **/
#define CCD_ROWS	  32		/**  No rows per chunk         **/
#define NCCDS		  4

#define BIAS_WIDTH        64
#define REFERENCE_WIDTH   BIAS_WIDTH
#define BITPIX            32


#ifndef	_RASINFO_DEFINED

#ifdef KPNO				/**  NEWFIRM @ KPNO	       **/
static pixArray rasInfo[] = {
    { "PanA",	1,	2},		/*  +-------------------------+	*/
    { "PanA",	2,	4},		/*  |  im3=B-2   |   im4=A-2  |	*/
    { "PanB",	1,	3},		/*  +-------------------------+	*/
    { "PanB",	2,	1}		/*  |  im1=B-1   |   im2=A-1  |	*/
};  					/*  +-------------------------+	*/
#endif

#ifdef CTIO				/**  NEWFIRM @ CTIO	       **/
static pixArray rasInfo[] = {
    { "PanA",	1,	3},		/*  +-------------------------+	*/
    { "PanA",	2,	1},		/*  |  im3=B-2   |   im4=B-1  |	*/
    { "PanB",	1,	2},		/*  +-------------------------+	*/
    { "PanB",	2,	4}		/*  |  im3=A-2   |   im4=A-1  |	*/
};  					/*  +-------------------------+	*/
#endif

#define _RASINFO_DEFINED
#endif
