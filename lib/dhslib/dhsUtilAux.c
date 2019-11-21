/*******************************************************************************
 * include(s):
 *******************************************************************************/
extern int dhsDebug;
#define DHSUTILAUX
#if !defined(_dhsUtil_H_)
 #include "dhsUtil.h"
#endif

#if !defined(_dhsImpl_H_)
 #include "dhsImplementationSpecifics.h"
#endif 

#include "dcaDhs.h"

/* dhsImpl_t impStruct; */
char dhsUtilDIR[DHS_MAXSTR];
char dhsUtilFILENAME[DHS_MAXSTR];
int dhsUtilFLAG = 0;
int dhsLabVersion = 1;
int dhsFileCreated = 0;
int dhsExtChar = 'a';
struct dhsMem *dhsMemP;

dhsNetw dhsNW, *dhsNWP;

dhsData dhs={0,""};

int procDebug = 60;
