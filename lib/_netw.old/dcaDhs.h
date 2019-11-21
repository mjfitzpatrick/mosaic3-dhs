/*

Low-Level Private Procedures


      dcaInitChannel  (dhsNW, type)
          dcaConnect  (dhsNW, devstr)

      dcaOpenChannel  (chan)
  dcaValidateChannel  (chan)
     dcaCloseChannel  (chan)

 dev = dcaGetSuperDev (dhsNW)
   dev = dcaGetDCADev (dhsNW)
    node = dcaDevNode (dev)
    port = dcaDevPort (dev)

	     dcaSend  (chan, option, value)
	      dcaSet  (dhsNW, option, value)
      value = dcaGet  (dhsNW, option)

     msg = dcaFmtMsg  (msgtype, whoami, addr, size)
     stat =  dcaSendMsg  (dhsNW, msg)
	  dcaReadMsg  (dhsNW, client_data)

	     dcaSend  (dhsNW, buf, size)
	     dcaRecv  (dhsNW, buf, size)

	   connectTo  (chan)

Utility Procedures/Macros:

  	 fd = DCS_FD (dhsNW)		DCS => Data Capture Supervisor
         fd = DCC_FD (dhsNW)		DCC => Data Capture Collector

*/

extern int procDebug;       /* use in DPRINTs debug macros */

/*---------------------------------------------------------------------------
Data Structures:
----------------------------------------------------------------------------*/
#ifndef XLONG
 #define XLONG int
#endif

#ifndef NULL
 #define NULL 0
#endif

#ifndef NO
 #define NO 0
#endif

#define SZMSG 120
#define SZNODE 120

#define SUPER_PORTNUMBER 4100     	/* default Supervisor port  	*/
#define SUPER_NODE       localhost     	/* default Supervisor node  	*/
#define COLL_PORTNUMBER  4101      	/* default Collector port  	*/ 
#define COLL_NODE        localhost     	/* default Supervisor node name */

typedef struct {
    double expID;          		/* exposure ID                  */
    char *obsSetID;        		/* observation set ID           */

    char dhsName[SZNODE]; 		/* application name 		*/ 
    char dhsSuperNode[SZNODE]; 		/* machine name for Supervisor  */ 
    int  dhsSuperPort;
    char dhsCollectorNode[SZNODE];	/* machine name for Collector   */ 
    int  dhsCollectorPort;

} dhsData, *dhsDataPtr;

dhsData      dhs;                    	/* global state struct          */

typedef struct msgHeader {
        int type;	/* message type, e.g. DCA_OPEN|DCA_SYS	  	*/
        uint whoami;	/* sender of data			  	*/
        double expnum;	/* exposure number                      	*/
	char obset[80]; /* observation set                              */
        int value;	/* set/get message value			*/
        int retry;	/* retry count for this message	  	  	*/
        int nbytes;	/* number of bytes of data following msg  	*/
        int checksum;	/* MD5 sum of data (for meta/pixel only)  	*/
	XLONG addr;
	int size;
} *msgHdrPtr, msgType, msgHeader;

struct dcaMessage {
    struct msgHeader *mheader;
    void *client_data;	/* data to be sent/read			  	*/
    int  *data_size;	/* size of data to be sent/read		  	*/
};

#define DCA_OPEN        000000001  /* Open something                       */
#define DCA_CLOSE       000000002  /* Close something                      */
#define DCA_READ        000000004  /* Read something                       */
#define DCA_WRITE       000000010  /* Write something                      */

#define DCA_INIT        000000020  /* Initialize something                 */
#define DCA_FAIL        000000040  /* Write something                      */
#define DCA_NOOP        000000100  /* NO-OP, used to request an ACK        */
#define DCA_ALIVE       000000200  /* Pixel data                           */

#define DCA_SET         000000400  /* Set something                        */
#define DCA_GET         000001000  /* Get something                        */

#define DCA_SYS         000002000  /* DHS System                           */
#define DCA_CONNECT     000004000  /* Connection to DHS collector  */
#define DCA_CON         000010000  /* Connection to DHS collector          */
#define DCA_EXP         000020000  /* Single Exposure                      */
#define DCA_META        000040000  /* Meta Data                            */

#define DCA_STICKY      000100000  /* Message Sticky Bit                   */

#define DCA_PIXEL       000100000 /* Pixel data                          */
#define DCA_OBS_CONFIG  000200000 /* observation set config structure    */
#define DCA_FP_CONFIG   000400000 /* focal plane config structure        */
#define DCA_MD_CONFIG   001000000 /* metadata config structure           */
#define DCA_EXPID       002000000 /* exposure ID                         */
#define DCA_OBSID       004000000 /* obs set ID                          */
#define DCA_EXP_OBSID   010000000 /* obs set ID                         */
#define DCA_DIRECTORY   020000000
#define DCA_FNAME       040000000

/* Set/Get options. */
#define DCA_ACK		2	/* ACK message             		     */
#define DCA_ERR	        -2	/* Error return             		     */
#define DCA_OK	        1       /* Okay return             		     */

#define DCA_IO_MODE     5       /* operational mode (live, simulation, etc)  */
#define DCA_IO_METHOD   6       /* communications method                     */
#define DCA_IO_SIZE     7       /* communications 'packet' size              */

#define DCA_CHECKSUM    8       /* compute checksum data for I/O? (boolean)  */
#define DCA_SUPDEV      10      /* supervisor "device" name                  */

#define DCA_NPANS       11      /* no. of PANs in use                        */
#define DCA_NCLIENTS    12      /* no. of clients connected to Supervisor    */
#define DCA_NCOLLECTORS		/* no. of collectors in use by Supervisor    */

#define DCA_SUPERVISOR	000030	/* Supevisor ID			*/
#define DCA_COLLECTOR	000040	/* Collector ID			*/

#define	DCA_DCADEV      21      /* Collector device string 		     */

#define DCA_DEBUG_LEVEL		/* debug level 				     */
#define DCA_SIMUL_LEVEL		/* simulation level			     */

#define DHS_SUPERVISOR  "DHS_SUPERVISOR" /* env variable for Supervisor machine */
/* Set/Get values. */
#define DCA_CAPTURE		/* I/O modes */
#define DCA_SIMULATION
#define DCA_DEBUG

#define DCA_FAST		/* I/O methods */
#define DCA_RELIABLE

#define DCA_NOCS
#define DCA_MSL
#define DCA_PAN

/* Prototype definitions */
char      *dcaActualHost      (void);
void       dcaCloseChannel    (struct dhsChan *);
int        dcaConnectionACK   (int);
char      *dcaDomain          (void);
msgHdrPtr  dcaFmtMsg          (XLONG, XLONG, ...);
void       dcaFreeDCAHost     (void);
char      *dcaGetDCADev       (int);
char      *dcaGetDCAHost      (void);
char      *dcaGetNode         (char *);
int        dcaGetPort         (char *);
int        dcaGetSimMode      (void);
char      *dcaGetSuperDev     (dhsNetw *);
int        dcaInitChannel     (dhsNetw *, XLONG);
void       dcaInitDCAHost     (void);
void       dcaOpenChannel     (struct dhsChan *, int *);
int        dcaRecv            (int, char *, int);
int        dcaRecvMsg         (int, char *, int);
int        dcaSend            (int, char *, int); 
int        dcaSendMsg         (int, msgHdrPtr);
int        dcaSendfpConfig    (int, XLONG, char *, int);
void       dcaSetDCAHost      (void);
void       dcaSetSimHost      (char *, int);
int        dcaSetSimMode      (int);
char      *dcaSimHost         (void);
int        dcaSimulator       (void);
void       dcaSockConnect     (int *, int, char *);
int        dcaSupCloseConn    (int);
int        dcaSupOpenConn     (char *, int, char *);
int        dcaSupOpenSocket   (char *, int);
int        dcaSupSendStatus   (int, char *);
int        dcaUseSim          (void);
int        dcaValidateChannel (dhsChan *);
/*
static int chan_read          (int, void *, int);
static int chan_write         (int, void *, int);
static int dcaSupParseNode    (char *, int, unsigned short *, unsigned long *);
static int dcaSupSockRead     (int, void *, int);
static int dcaSupSockWrite    (int, void *, int);
*/
