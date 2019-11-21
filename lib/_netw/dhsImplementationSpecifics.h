#ifndef _dhsImpl_H_
 #define _dhsImpl_H_ 1.1.0
 #define _dhsImpl_D_ "11 January, 2008."
 #define _dhsImpl_S_ "1.1.0"
 #define _dhsImpl_A_ "P. N. Daly, N. C. Buchholz"
 #define _dhsImpl_C_ "(c) AURA Inc, 2004. All rights reserved."

 #ifndef XLONG
  #define XLONG int
 #endif

 /******************************************************************************
  * define(s)
  ******************************************************************************/
 #define DHS_IMPLEMENTATION "NETW"
 #define DHS_IMPL_MAXSTR       128

 /******************************************************************************
  * typedef(s)
  ******************************************************************************/
 typedef struct dhsChan {
  int   fd;                   /* socket descriptor for connection              */
  char *device;               /* the 'node:port' string                        */
  char *node;                 /* node where DCA object is running              */
  char *name;                 /* name of client application		       */
  int   port;                 /* port on node for DCA object                   */
  int   nerrs;                /* error count on channel                        */
  int   nresends;             /* packet resend count on channel                */
  int   retries;              /* needed at all??                               */
  int   timeout;              /* needed at all??                               */
  int   initialized;          /* needed at all??                               */
  int   connected;            /* connection status flag                        */
  int   type;                 /* channel type                                  */
 } dhsChan;

 typedef struct dhsNetw {
  struct dhsChan  *super;     /* Supervisor channel                            */
  struct dhsChan  *collector; /* Collector channel                             */
  int    nopen;               /* No. of open channels                          */
 } dhsNetw;

 extern dhsNetw dhsNW;

 /*******************************************************************************
 * prototype(s)
 ******************************************************************************/
 int dcaSimulator( void );

 /*******************************************************************************
  * variable(s) 
  ******************************************************************************/
 #ifndef DHSUTILAUX
  extern char dhsUtilDIR[256];
  extern char dhsUtilFILENAME[256];
  extern int dhsUtilFLAG;
  extern int dhsFileCreated;
  extern int dhsExtChar;
 #endif
#endif

extern int dhsDebug;
