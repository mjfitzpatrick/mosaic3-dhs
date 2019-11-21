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


// assume these are CCD withs including bias overscan in each direction
#define CCD_W		  2112		/**  width		       **/
#define CCD_H		  2112		/**  height		       **/
#define CCD_BPP		  4		/**  bytes per pixel	       **/
#define CCD_ROWS	  32		/**  No rows per chunk         **/
#define NCCDS		  16		/**  No of CCDs		       **/
#define NAMPS_W		  1		/**  No of amplifiers	       **/
#define NAMPS_H		  1		/**  No of amplifiers	       **/

#define BIAS_WIDTH        47
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
    { "PanA",   1,      5},            
    { "PanA",   2,      6},
    { "PanA",   3,      1},
    { "PanA",   4,      2},
    { "PanA",   5,      7},            
    { "PanA",   6,      8},
    { "PanA",   7,      3},
    { "PanA",   8,      4},
    { "PanA",   9,      13},            
    { "PanA",   10,     14},
    { "PanA",   11,     9},
    { "PanA",   12,     10},
    { "PanA",   13,     15},            
    { "PanA",   14,     16},
    { "PanA",   15,     11},
    { "PanA",   16,     12}
};

/*
   +-----------------------+ 
   | A9  | A10 | A13 | A14 |
   +-----------------------+ 
   | A11 | A12 | A15 | A16 |
   +-----------------------+ 
   | A1  | A2  | A5  | A6  |
   +-----------------------+ 
   | A3  | A4  | A7  | A8  |
   +-----------------------+ 
*/


#define _RASINFO_DEFINED
#endif
