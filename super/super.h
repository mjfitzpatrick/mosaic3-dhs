/*
 *  SUPER.H -- Global definitions for the DHS Supervisor.
 */


#define	DEBUG			1

#ifdef __DARWIN__
#define MACOSX
#endif


/*
*/
#include "cdl.h"
#include "mbus.h"
#include "smCache.h"



/* For 64-bit compatability.
*/
#ifndef XLONG
#define XLONG int
#endif


/* Default values, size limiting values.
 */
#define	MAX_CLIENTS		8	/* max display server clients	*/
#define	DEF_INTERVAL		1.0	/* default polling time		*/
#define	DEF_FBCONFIG		41	/* stdimage = imt4400		*/
#define	DEF_FBWIDTH		8800
#define	DEF_FBHEIGHT		8800
#define	DEF_FRAME		1
#define	DEF_CACHE		"\0"
#define	DEF_GUI			"default"

#ifndef SZ_NAME
#define	SZ_NAME			80	/* file name buffer		*/
#endif
#ifndef SZ_PATH
#define	SZ_PATH			164	/* pathname buffer		*/
#endif
#ifndef	SZ_FNAME
#define	SZ_FNAME		64
#endif
#ifndef	SZ_LINE
#define	SZ_LINE			1024
#endif
#ifndef	ERR
#define	ERR			(-1)
#endif
#ifndef	OK
#define	OK			0
#endif
#ifndef	EOS
#define	EOS			'\0'
#endif


#define	SZ_IMTITLE		128	/* image title string		*/
#define	SZ_WCTEXT		80	/* WCS box text 		*/
#define	SZ_WCSBUF		1024	/* WCS text buffer size		*/
#define	SZ_MSGBUF		8192	/* message buffer size		*/


/* Magic numbers for display server connection.
 */
#define DEF_DISP_PORT	   5137		/* default tcp/ip socket	*/
#define DEF_DISP_HOST	   "localhost"	/* default host			*/

/* Magic numbers for public spervisor connection.
 */
#define DEF_PORT	   4150		/* default tcp/ip socket	*/
#define DEF_HOST	   "localhost"	/* default host			*/
#define DEF_CONFIG	   "/dhs/lib/DHS.cfg"	/* default config file	*/

#define DEF_SEQNO	   0
#define DEF_ROOT	   "image"
#define DEF_RDIR	   "/home/data"
#define DEF_PDIR	   "/home/data"



/*****************************************************************************
 *
 *  Application resources and runtime descriptors.
 *  ----------------------------------------------
 *
 *  We set the initial state at Supervisor startup, from a combination of
 *  defaults and user-defined resources.  The Supervisor passes these values
 *  into any newly created clients, thereafter these are the values maintained
 *  in the GUI.
 *
 ****************************************************************************/

/* Image display/RTD resources. 
 */
typedef struct {
    String disp_node;			/* display server node 		*/
    int   def_config;               	/* default FB config            */
    int   disp_frame;               	/* current display frame        */
    int   disp_enable;              	/* RTD enable flag              */
    int   stat_enable;              	/* pixel stats enable flag      */
    int   rot_enable;              	/* rotate enable flag      	*/
    int   otf_enable;              	/* OTF enable flag      	*/
    int   lo_gain;              	/* loGain mode flag      	*/
    int   par_enable;              	/* parallel enable flag      	*/

    float poll_interval;            	/* display polling interval     */

    int   stdimage;                 	/* display frame buffer         */
    int   width, height;            	/* FB width, height             */
    Boolean invert;                 	/* use inverted colormap        */
} dhsRTD, *dhsRTDPtr;



/*****************************************************************************
 *  Shared Memory Cache state.
 */
typedef struct {

    smCache_t *_smc;			/* SMC struct pointer		*/

    float poll_interval;            	/* display polling interval     */
    char  cache_file[SZ_FNAME];		/* SMC cache file		*/

} dhsSMC, *dhsSMCPtr;


/*  The state of an individual cache.  Each cache will normally reside on 
 *  a separate machine, this struct collects the information from each node
 *  for display in the GUI.
 */
typedef struct {
    String  created;			/* timestamp of cache creation	*/
    String  updated;			/* timestamp of cache update	*/
    String  cache_file;			/* cache lock file		*/
    String  node;			/* node name			*/
     key_t  memKey;			/* memory segment key		*/
       int  shmid;			/* segment id			*/
     XLONG  memUsed;			/* memory used			*/
     XLONG  memAvail;			/* memory available		*/
       int  nprocs;			/* num. processes attached	*/
       int  segUsed;			/* num. segments used		*/

      char *segmenLog;			/* Segment logfile (formatted)	*/

} smcState, *smcStatePtr;


typedef struct {
    String node;			/* node name			*/
    String cmd;				/* command path			*/
} dhsProc, *dhsProcPtr;



/*****************************************************************************
 *  Client (PAN) I/O channel. 
 */

typedef struct {
    XtPointer sup;                  	/* backpointer to sup descriptor */
    XtPointer id;                   	/* input callback id             */
    int  datain;                     	/* input channel                 */
    int  dataout;                    	/* output channel                */
    int  connected;                  	/* client connected?             */
    int  port;                  	/* port # of this channel 	 */
    int  pid;                  		/* pid associated w/ this channel*/
    char name[SZ_FNAME+1];          	/* client name                   */
    char host[SZ_FNAME+1];          	/* host name                     */
    char ip_addr[SZ_FNAME+1];          	/* IP addr                       */
    char msgbuf[SZ_MSGBUF+1];       	/* incomplete message buffer     */
} clientIoChan, *clientIoChanPtr;



/*****************************************************************************
 * Command Paths, Ports, Nodes, etc.
 */
typedef struct {
    String cmd;				/* command path			*/
       int port;			/* inet port 			*/
    String node;			/* node name			*/
} cmdInfo, *cmdInfoPtr;

typedef struct {
    cmdInfo  super;			/* DHS Supervisor		*/
    cmdInfo  rtd;			/* Real-Time Display		*/
    cmdInfo  dca;			/* Data Capture Agent		*/
    cmdInfo  collector;			/* Data Collector		*/
    cmdInfo  picfeed;			/* Data Feeder			*/
} dhsCmd, *dhsCmdPtr;


/*****************************************************************************
 *
 */

#define SZ_PNAME        12
#define SZ_PHOST        32
#define SZ_PDATE         9
#define SZ_PSTAT        32

#define SZ_PROCTABLE    128


/* CLIENT PROCESS DESCRIPTORS	
**********************************/
typedef struct {
    char    name[SZ_PNAME];             /* client process name          */
    char    host[SZ_PHOST];             /* client host name             */
    pid_t   pid;                        /* process id                   */
    int     port;                       /* inet port used               */
    int     tid;                        /* msgbus tid                   */
    char    colID[SZ_PNAME];            /* assigned collector ID	*/

    char    date[SZ_PDATE];             /* time of status message       */
    char    status[SZ_PSTAT];           /* status string                */

    clientIoChan chan;          	/* channel used by this proc	*/
    int     connected;                  /* is the process connected?    */
    int     active;                     /* is the process active?     	*/
} clientProc, *clientProcPtr;



/* CONFIGURATION FILE TASKS	
**********************************/
typedef struct {			
    char    name[SZ_PNAME];             /* client process name          */
    char    host[SZ_PHOST];             /* client host name             */
    char    command[SZ_LINE];           /* client command string        */
} confTask, *confTaskPtr;



/* KEYWORD MONITORING
**********************************/

#define MAX_KEYWMON      	32
#define KEYW_LEN		64

typedef struct {			
    char    keyw[KEYW_LEN];             /* keyword to monitor		*/
    char    dbname[KEYW_LEN];           /* database to monitor (or all)	*/
} keywMon, *keywMonPtr;

keywMon keywList[MAX_KEYWMON];
int	NKeywords;




/* PAN-COLLECTOR PAIRINGS	
**********************************/
typedef struct {			
    clientProcPtr pan;			/* PAN side of connection	*/
    clientProcPtr collector;		/* Collector side of connection */
    int     inUse;			/* pairing made?		*/
    XLONG   bytesTransferred;		/* amount of data transferred	*/
    double  bytesTotal;			/* total transfer on conn (GB)	*/
    int     units;			/* units of transfer stats	*/
    char    timestamp[SZ_PDATE];        /* timestamp of last transfer	*/
    double  expID;			/* current exposure ID		*/
    char    obsSetID[SZ_LINE];		/* current observation ID	*/
} panConn, *panConnPtr;

typedef struct {			
    char    phost[SZ_PHOST];            /* pan host			*/
    char    chost[SZ_PHOST];            /* collector host		*/
    int     socket;			/* collector socket (optl)	*/
} panPair, *panPairPtr;

typedef struct {			
    char    name[SZ_PHOST];             /* host name			*/
    char    ip_addr[SZ_PHOST];          /* IP address (string)		*/
} hostTable, *hostTablePtr;




/* PROCESSING QUEUE
**********************************/

#define	SZ_PROC_QUEUE		512
#define	PQ_AVAILABLE		0
#define	PQ_START		1
#define	PQ_READY		2
#define	PQ_ACTIVE		3
#define	PQ_DONE			4


struct qnode {
    double  expID;			/* current exposure ID		*/
    int     status;			/* status of jobs in queue	*/
    char    t_in[SZ_PDATE];        	/* time went into the queue	*/
    char    t_out[SZ_PDATE];        	/* time went out for processing */
    char    t_done[SZ_PDATE];        	/* time when all done		*/
    struct qnode   *next;
} ;
typedef struct qnode pqNode;
typedef struct qnode *pqNodePtr;

/*****************************************************************************
 * Main DHS Supervisor Application Resources.
 */
typedef struct {
    String gui;				/* GUI file name 		*/
    String host;			/* host for INET socket	 	*/
    int    port;			/* port for INET socket	 	*/
    String config;			/* DHS config file		*/
    String cache_file;			/* default SMC cache file 	*/

    int    debug;			/* debug level			*/
    int    verbose;			/* verbose toggle		*/
    int    showActivity;		/* show process activity	*/

    float stat_interval;		/* status polling interval 	*/

    char   fileRoot[SZ_FNAME];		/* output image root name	*/
    char   rawDir[SZ_FNAME];		/* raw image directory		*/
    char   procDir[SZ_FNAME];		/* processed image directory	*/
    int    imgSeqNo;			/* image sequence number	*/
    char   userStr[SZ_FNAME];		/* user directory suffix	*/

    String imtdev;			/* image display device		*/

    int    mytid;			/* message bus tid		*/
    int    dca_tid;			/* DCA tid			*/
    int    nbin;			/* DHS binning factor		*/
    int    nclients;			/* no. of client tasks started	*/
    int    nready;			/* no. of client ready to run	*/

    int    use_console;			/* Use Console mode from Super?	*/
    int    use_cdl;			/* Use CDL from Super?		*/
    CDLPtr sup_cdl;			/* CDL descriptor struct	*/

    int    use_smc;			/* Use SMC from Super?		*/
    dhsSMC sup_smc;			/* SMC descriptor struct	*/

    dhsRTD  rtd;			/* RTD descriptor struct	*/
    dhsCmd  cmd;			/* CMD descriptor struct	*/
    dhsProc proc;			/* Process descriptor struct	*/

    int        numClients;              /* No. clients in proctable     */
    int        numProcMgrs;             /* No. of SMC Managers		*/
    clientProc procTable[SZ_PROCTABLE]; /* The process table            */

    int        numConfTasks;            /* No. tasks in config table    */
    confTask   confTable[SZ_PROCTABLE]; /* The configuration table      */

    int        numPanConns;             /* No. of pan-collector conns	*/
    panConn    connTable[SZ_PROCTABLE]; /* The connection table         */

    int        numPanPairs;             /* No. of machine pairings	*/
    panPair    pairTable[SZ_PROCTABLE]; /* The pairing host table       */

    int        numHostTable;             /* No. of machines		*/
    hostTable  hosts[SZ_PROCTABLE];      /* The host name table         */

    char       triggerHost[SZ_LINE];	/* Processing trigger hIst	*/

    clientIoChan    pub_chan;          	/* public client inet port	*/
    clientIoChan    pub_client[32];    	/* client i/o descriptors   	*/
    int             chan_offset;        /* channel offset		*/
    clientIoChanPtr mbus_chan;         	/* message bus "channel"	*/
    clientIoChanPtr stdin_chan;        	/* stdin (console) "channel"	*/

    pqNode	queue[SZ_PROC_QUEUE];	/* processing queue		*/
    pqNodePtr	qFirst;
    pqNodePtr	qLast;
    int		qCount;

    /* Internal state. */
    XtPointer obm;			/* object manager 		*/
    Widget    toplevel;			/* dummy toplevel app shell 	*/

    int *clientPrivate;			/* used by supervisor client 	*/

    /* PVM Message Bus. */

} supData, *supDataPtr;



/* Client callback struct. */
typedef struct {
        supDataPtr sup;
        Tcl_Interp *tcl;
        Tcl_Interp *server;
} supClient, *supClientPtr;




/****************************************************************************/

#ifdef SUPER_MAIN
supData super_data;

#define	XtNgui			"gui"
#define	XtCGui			"Gui"
#define	XtNconfig		"config"
#define	XtCConfig		"Config"
#define	XtNport			"port"
#define	XtCPort			"Port"
#define	XtNhost			"host"
#define	XtCHost			"Host"

static XtResource resources[] = {
    {
	XtNgui,
	XtCGui,
	XtRString,
	sizeof(String),
	XtOffsetOf(supData, gui),
	XtRImmediate,
	(caddr_t)"default"
    },
    {
	XtNconfig,
	XtCConfig,
	XtRString,
	sizeof(String),
	XtOffsetOf(supData, config),
	XtRImmediate,
	(caddr_t)"default"
    },
    {
	XtNport,
	XtCPort,
	XtRInt,
	sizeof(int),
	XtOffsetOf(supData, port),
	XtRImmediate,
	(caddr_t)DEF_PORT
    },
    {
	XtNhost,
	XtCHost,
	XtRString,
	sizeof(String),
	XtOffsetOf(supData, host),
	XtRImmediate,
	(caddr_t)DEF_HOST
    },
};



#else

extern supData    super_data;
extern XtResource resources[];

#endif

/****************************************************************************
 *  Utility Functions.
 */
#ifndef abs
#define	abs(a)		(((a)<0)?(-(a)):(a))
#endif
#ifndef min
#define min(a,b)	((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b)	((a)<(b)?(b):(a))
#endif
#ifndef nint
#define nint(a)		((a)<(0)?((int)(a-0.5)):((int)(a+0.5)))
#endif

void 	  sup_initialize(), sup_reset(), sup_refresh();
void 	  sup_close(), sup_alert();
void 	  sup_status(), sup_error();
void 	  sup_message(), sup_msgi();

/*
XtPointer sup_addInput();
void 	  sup_removeInput();
*/

void 	  sup_clientOpen(), sup_clientClose();
int 	  sup_clientExecute();


/****************************************************************************
 *  Functions Prototypes
 */

/*  supClient.c
 */
void sup_clientOpen (supDataPtr sup);
void sup_clientClose (supDataPtr sup);
int  sup_clientExecute (supClientPtr xc, Tcl_Interp *tcl, char *objname,
                        int key, char *command);

/*  supConfig.c
 */
void supConfigure (supDataPtr sup);


/*  supIO.c
 */
void  supInitIO (supDataPtr sup);
XtPointer sup_addInput (supDataPtr sup, int input, void (*proc)(), 
		XtPointer client_data);
void  sup_removeInput (supDataPtr sup, XtPointer id);
void  mbusMsgHandler (void *data, int fd, int id);
void  stdinMsgHandler (void *data, int fd, int id);
void  supIOHandler(supDataPtr sup, int from, int subject, char *msg);
void  logMessage (supDataPtr sup, int from, int to, char *msgtext);
void  inputEventHandler (char *cmd);
char *mytok(char *str, int N);
int   supHostAdded (int mid);
int   supTaskExited (int mid);
int   supHostDeleted (int mid);


/*  supKeyw.c
 */
void  supUpdateKeywDB (char *list);
void  printKeywList (double expID);
char *supMakeKeywordList ();


/*  supPanHandler.c
 */
void  superHandleClient (supDataPtr sup, int socket, char *msg);
#ifdef USE_TRIGGER_HOST
int   isTriggerHost (supDataPtr sup, int clid);
#endif
void  superProcNext (supDataPtr sup);


/*  supProc.c
 */
int   procAddClient (supDataPtr sup, clientIoChanPtr chan, char *name,
    	  char *host, int port, int tid, pid_t pid, char *status, int update);
int   procDelClient (supDataPtr sup, int clid, int update);
int   procFindClient (supDataPtr sup, int port, int tid);
int   procFindBySocket (supDataPtr sup, int socket_fd);
void  procDisconnect (supDataPtr sup, int clid);
void  procCleanTable (supDataPtr sup);
int   procUpdateStatus (supDataPtr sup, int clid, char *status, int update);
int   procSetValue (supDataPtr sup, int clid, char *msg, int update);
int   procGetValue (supDataPtr sup, int clid, char *msg, int update);
void  procUpdateProcessTable (supDataPtr sup);
void  procUpdateTransferTable (supDataPtr sup);
void  procBreakoutClientID (char *id, char *name, char *host, int *pid);
char *procTimestamp (void);


/*  supQueue.c
 */
void   pqInit (supDataPtr sup);
void   pqAdd (supDataPtr sup, double expID);
double pqNext (supDataPtr sup);
void   pqSetStat (supDataPtr sup, double expID, int status);
int    pqGetStat (supDataPtr sup, double expID);
int    pqDataWaiting (supDataPtr sup);
void   pqPrintQ (supDataPtr sup);
void   pqTest (supDataPtr sup);


/*  supSocket.c
 */
clientIoChanPtr  supOpenInet (supDataPtr sup);
void   supCloseInet (supDataPtr sup);
void   sup_connectClient (clientIoChanPtr chan, int *source, XtPointer id);
void   sup_disconnectClient (clientIoChanPtr chan);
void   supClientIO (clientIoChanPtr chan, int *fd_addr, XtInputId *id_addr);
char  *supNeuter (char *name);
clientIoChanPtr  clientGetChannel (supDataPtr sup);
int    supMsgType (char *message);
int    clientSockRead (int fd, void *vptr, int nbytes);
int    clientSockWrite (int fd, void *vptr, int nbytes);
char  *supHostLookup (supDataPtr sup, char *name);


/*  super.c
 */
void  sup_initialize (supDataPtr sup);
void  sup_status (char *msg);
void  sup_error (char *msg);
void  sup_message (supDataPtr sup, char *object, char *message);
void  sup_msgi (supDataPtr sup, char *object, int value);
void  sup_messageFromFile (supDataPtr sup, char *object, char *fname);
void  sup_alert (supDataPtr sup,char *text,char *ok_action,char *cancel_action);
void  supSetDebug (char *msg);
void  supDebugFile (char *file, int value);
void  sup_shutdown (supDataPtr sup);
int   supSeqNo (int seqnum);
char *supSeqName (char *name);

