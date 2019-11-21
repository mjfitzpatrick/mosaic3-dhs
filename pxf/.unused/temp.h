/**
 *  DETECTOR.H -- Detector definitions for Mosaic 3  
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


#define CCD_W		  4196		/**  width		       **/
#define CCD_H		  4096		/**  height		       **/
#define CCD_BPP		  4		/**  bytes per pixel	       **/
#define CCD_ROWS	  32		/**  No rows per chunk         **/
#define NCCDS		  4		/**  No of CCDs		       **/
#define NAMPS_W		  2		/**  No of amplifiers	       **/
#define NAMPS_H		  2		/**  No of amplifiers	       **/

#define BIAS_WIDTH        50
#define REFERENCE_WIDTH   BIAS_WIDTH
#define BITPIX            32


#ifndef _RASINFO_DEFINED

#if 0
static pixArray rasInfo[] = {
    { "PanA",   1,      4},             /*  +-------------------------+ */
    { "PanA",   2,      3},             /*  |      |     |      |     | */
    { "PanA",   3,      2},             /*  |  A5  | A6  |  A7  |  A8 | */
    { "PanA",   4,      1},             /*  +-------------------------+ */
    { "PanA",   5,      8},             /*  |      |     |      |     | */
    { "PanA",   6,      7},             /*  |  A4  | A3  |  A2  |  A1 | */
    { "PanA",   7,      6},             /*  +-------------------------+ */
    { "PanA",   8,      5}
};
#endif

static pixArray rasInfo[] = {
    { "PanA",   1,      2},            
    { "PanA",   2,      1},
    { "PanA",   3,      4},
    { "PanA",   4,      3}
};

/* +-----------------------+ 
   |   A4      |   A3      |
   |           |           |
   +-----------------------+ 
   |   A2      |   A1      |
   |           |           |
   +-----------------------+ */


#define _RASINFO_DEFINED
#endif
