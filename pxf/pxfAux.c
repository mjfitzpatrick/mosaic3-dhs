/*******************************************************************************
 * include(s):
 ******************************************************************************/

extern int dhsDebug;
#define DHSUTILAUX
#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif

#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif

/*dhsImpl_t impStruct; */
char 	pxfDIR[DHS_MAXSTR];
char 	pxfFILENAME[DHS_MAXSTR];
int 	dhsLabVersion   	= 1;
int 	pxfFLAG 	    	= 3;
int 	pxfFileCreated  	= 0;
int 	pxfExtChar      	= 'a';
int 	procDebug       	= 0;
struct 	dhsMem *dhsMemP;
