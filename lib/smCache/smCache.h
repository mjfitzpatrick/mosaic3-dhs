/*
**  SMCACHE -- Shared Memory Cache Interface.
**
**/


/*
**  Dependent include files.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>


#ifdef MACOSX
typedef unsigned long 	ulong;
#endif
#ifndef XLONG
#define XLONG		unsigned int
#endif


/*
**  Defined Constants
*/
#ifndef SZ_PATH
#define SZ_PATH		164		/* sizes and limits		*/
#endif
#ifndef SZ_FNAME
#define SZ_FNAME	64
#endif
#ifndef SZ_LINE
#define SZ_LINE		1024
#endif
#define SZ_KEYW		64
#define SZ_VAL		64

#define MAX_SEGS	4096		/* max npages we can manage  	*/
//#define O_MAX_SEG_SIZE 33550336        /* = (32MB - 4096) bytes        */
//#define MAX_SEG_SIZE  67104768        /* = (64MB - 4096) bytes        */
#define O_MAX_SEG_SIZE  67104768        /* = (64MB - 4096) bytes        */
#define MAX_SEG_SIZE    134213632       /* = (128MB - 4096) bytes       */

#define MAX_PROCS	8		/* max concurrent procs 	*/


#define TY_NONE		-1		/* no types			*/
#define TY_ANY		0		/* any type			*/
#define TY_VOID		1		/* generic type			*/
#define TY_DATA		2		/* pixel data type		*/
#define TY_META		3		/* meta-data type		*/

#ifndef TRUE				/* usual bool suspects		*/
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif
#ifndef OK
#define OK              0
#endif
#ifndef ERR
#define ERR             -1
#endif



/*
** Default Config Parameters 
#define SMC_DEBUG	    0
*/
#define SMC_DEBUG  (getenv("SMC_DEBUG")!=NULL||access("/tmp/SMC_DEBUG",F_OK)==0)

#define AC_LOCK		    001		    /* access lock flag	   	      */
#define VM_LOCK		    002		    /* VM lock flag		      */

#define	DEF_CACHE_FILE	    "/tmp/.smc%d"
#define DEF_CACHE_SIZE	    8192	    /* 2 system pages 	   	      */
#define DEF_DIRPATH	    "./"
#define DEF_FROOT	    "image"
#define	DEF_NSEGS	    32		    /* no of managed segs 	      */
#define DEF_MIN_SEG_SIZE    4096	    /* 1 system page (usually) 	      */
#define DEF_POLL_INTERVAL   500		    /* reader poll every 1/2 second   */
#define DEF_THROTTLE_TIME   1000	    /* writer pause every 1 second    */
#define DEF_THROTTLE_NTRY   60		    /* wait 1 min before failing      */
#define DEF_LOCK_CACHE	    (AC_LOCK|VM_LOCK)
#define DEF_LOCK_SEGS	    (AC_LOCK|VM_LOCK)
#define DEF_VERBOSE	    TRUE



/*
**  Globals
*/
#ifdef SM_UNIT_TEST
char 	cache_path[SZ_PATH];			/* Config options	*/
int  	npages;
int  	page_size;
int  	lock_cache;
int  	lock_page;
#endif


typedef struct smCache		smCache_t;

/*  Runtime cache configuration parameters.
**
**      The 'cache_path' is the lockfile used for the shmem object which 
**  will hold the cache parameters for other processes that need to attach
**  to the cache, as well as a history of the pids that mapped the cache.
**  If not specified a default based on the user-id is used, otherwise the
**  user-specified value names the cache to be used.
**      If specified, 'cache_size' defines the size of the public cache
**  object and the number of segments requested is derived from this value
**  and any supplied value ignored.  Otherwise, the cache_size is computed
**  from the number of requested segments or the defaults.  The minimum 
**  segment size is one physical memory page or the user specified value.
**  By default both the cache and the managed segments are locked in memory.
**
*/
struct sysConfig {
    smCache_t	*smc;		    /* backptr to main cache struct	*/

    char  config[SZ_PATH];	    /* config string			*/
    char  cache_path[SZ_PATH];	    /* cache file			*/
    int   cache_size;		    /* requested cache size		*/
    key_t cache_memKey;		    /* cache key			*/
    void *cache_addr;		    /* requested cache size		*/

    int   debug;		    /* debug flag			*/
    int   nsegs;		    /* No. of managed segments		*/
    int   max_segs;		    /* Max No. of managed segments	*/
    int   min_seg_size;		    /* minimum segment size		*/
    int   lock_cache;		    /* cache lock flags			*/
    int   lock_segs;		    /* segment lock flags		*/

    XLONG throttle_time;	    /* getPage throttle delay (millisec)*/
    XLONG throttle_ntry;	    /* getPage throttle max attempts	*/

    XLONG interval;		    /* polling interval (millisec)      */
    int	  verbose;		    /* verbosity flag			*/

    /*  Runtime system resource parameters.  These are machine-dependent
    **  values derived from a sysconf() system call.
    */
    XLONG sys_page_size;	    /* system page size 		*/
    XLONG sys_phys_pages;	    /* No. system pages 		*/
    XLONG sys_avphys_pages;	    /* No. available system pages 	*/
    XLONG sys_pmask;		    /* system page mask 		*/
};
typedef struct sysConfig	sysConfig_t;


#ifndef _dhsUtil_H_

/***************************************************************************
 * TEMPORARY DEFINITIONS UNTIL WE USE dhsUtil.h IN THE CODE
 **************************************************************************/
/* Focal plane configuration structure.
 */
struct fpConfig { 
   XLONG xSize;   /* size of a row on the focal plane (number of columns) */
   XLONG ySize;   /* size of a column on the focal plane (number of rows) */
   XLONG xStart;  /* column index of the first pixel in this part of plane */
   XLONG yStart;  /* row index of the first pixel in this part of plane */
   XLONG dataType;/* data type of the pixels (number of bytes) */
   XLONG xDir;    /* direction of increasing (later time) column number */
   XLONG yDir;    /* data sent from monsoon places the first pixel received from
                     the DHE as pixel 1,1 in the buffer and assume it to be in
                     the lower left corner of the detector */
   XLONG xDetSz;  /* size of a single detector row (Number of pixel columns)  */
   XLONG yDetSz;  /* size of a single detector column (Number of pixel rows)  */
   XLONG xDetCnt; /* size of a mosaic row number of columns of detectors      */
   XLONG yDetCnt; /* size of a mosaic column number of rows of detectors      */
   XLONG xAmpCnt; /* number of readout amps per row of a detector              */
   XLONG yAmpCnt; /* number of readout amps per col of a detector              */
} ; typedef struct fpConfig 	fpConfig_t;


/* Meta data configuration structure.
*/
#define DHS_MAXFIELDS            4096 	/* maximum number of fields 	*/

struct mdConfig { 		   
   XLONG metaType;                 /* conceptual type of the meta data	*/
   XLONG numFields;                /* no. of fields in meta data array  */
   XLONG fieldSize[DHS_MAXFIELDS]; /* no. of items in the field 	*/
   XLONG dataType[DHS_MAXFIELDS];  /* data type of the values in field  */

} ; typedef struct mdConfig 	mdConfig_t;


/***************************************************************************
 * END TEMPORARY DEFINITIONS
 **************************************************************************/
#endif

/* Cache state info structure.
 */

struct s_info {
    void        *smc;                   /* backpointer to SMC (local addr) */
    pid_t       pid;                    /* pid of attached process (index) */
    double      atime;                  /* time pid attached               */
    double      ltime;                  /* time pid last modified          */
    double      utime;                  /* time pid last checked for update*/
    int         attached;               /* pid is currently attached	   */
    int         reader;                 /* segments process will read	   */
    void        *addr;                  /* address of attachment in the    */
                                        /* local process address space     */
}; typedef struct s_info  smState_t;



/*  smSegment sructure definition.  A Segment is allocated with a request
**  for a particular size we assume refers to the data area.  The segment
**  is actually resized to be large enough to hold the self-identifying 
**  header structure before the data.  The 'header' in this case contains
**  a 'data' pointer that may be used to access the start of the data area.
**
*/
struct smcSegment {
    /*  General Segment Header fields.
     */
    smCache_t	*smc;		  /* backpointer to SMC main structure	*/
    int		pagenum;	  /* SMC page number			*/
    key_t  	memKey;           /* Segment shared mem id key          */
    long    	shmid;		  /* shmid				*/

    XLONG  	size;             /* Size of entire segment area (bytes)*/
    int    	type;             /* Segment type (META, DATA, OTHER)   */
    double  	ctime;            /* Epoch segment was created		*/

    /* NEWFIRM-specific fields.
     */
    XLONG  	who;              /* who sent the data                  */
    double 	expID;            /* exposure ID                        */
    char   	obsetID[SZ_PATH]; /* observation set ID                 */

    char	dirpath[SZ_PATH]; /* current working directory		*/
    char	froot[SZ_PATH];   /* filename root			*/
    int		seqno;		  /* image sequence number		*/
    char	colID[SZ_PATH];	  /* collector ID name			*/
    int		expPageNum;	  /* page num. w/in this exposure	*/

    mdConfig_t	mdCfg;		  /* metadata configuration struct	*/
    fpConfig_t	fpCfg;		  /* focal plane configuration struct	*/

    pthread_mutex_t mut;	  /* thread lock (unused)		*/

    XLONG  	dsize;            /* Size of segment data (bytes)       */
    void        *data;		  /* User data area			*/
    char        *edata;		  /* End of User data area (sentinal)	*/
};
typedef struct smcSegment	smcSegment_t;



/* smcPage structure definition.  A "Page" is a shared memory segment that
** is comprised of a descriptive header and an arbitrarily long userdata
** area.  Pages are allocated in units of the system page size and so may
** have padding at the end.  The start of the user-data is word aligned.
**/
struct smcPage {
    smCache_t	*smc;		  /* backpointer to main cache struct	*/
    key_t  	memKey;           /* Segment shared mem id key          */
    long    	shmid;		  /* shmid				*/
    char 	lockFile[SZ_PATH];/* cache file				*/
    XLONG  	size;             /* Size of segment (bytes)            */
    int    	free;             /* Segment free flag?			*/

    void	*seg_addr;	  /* address of segment			*/

    int    	type;             /* Segment type (META, DATA, OTHER)   */
    double  	ctime;            /* Epoch page was created		*/
    pid_t    	cpid;             /* Page creator pid			*/
    int    	nattached;        /* No. of attached processes          */
    int    	refcount;         /* reference count                    */
    int    	nreaders;         /* no. of readers interested in segs	*/
    int    	initialized;      /* Segment initialized?		*/

    double 	expID;            /* exposure ID                        */
    char	colID[SZ_PATH];	  /* collector ID name			*/
    int    	ac_locked;        /* Is Segment locked for access?	*/
    int    	vm_locked;        /* Is Segment locked in VM?        	*/

    pthread_mutex_t mut;	  /* thread lock (unused)		*/

    smState_t   pstate[MAX_PROCS];/* process state lookup table		*/

    smcSegment_t *segment;	  /* the shmem segment itself		*/
};
typedef struct smcPage		smcPage_t;




/*****************************************************************************
** Data Structures
******************************************************************************
**
**  The main shared memory cache (SMC) structure.  
**  
**      The SMC is a shared memory object used to manage "segments" of shared
**  memory data.  'smCache' is the one public entry point into the cache and
**  the only public object.  Clients needing a 'segment' for data use an API
**  method which either allocates a new managed segment or returns an existing
**  segment of the correct size.  The smCache struct is then updated to reflect
**  the current list of managed segments and the number of references to each.
**  Only one process may initiate the SMC, new processes thereafter simply
**  attach to the public object and the API will attach the managed segments
**  for them.
**  
**  A "page" is a reference within the SMC public area that acts as a
**  pointer to a segment.  That is, the SMC that all processes attach to has
**  a global runtime struct containing the config info for the cache and a
**  linked-list of segments (i.e. 'pages') managed by the cache.  The 'page'
**  is a structure describing the segment for the interface, i.e. marking it
**  as 'free' or 'locked' for processes interested in the managed segments.
**  SMC simply requests a segment of a particular size, and if the SMC has
**  Various API routines will trigger the SMC to be updated with the current
**  state of the managed segments before proceeding.
**  
**  Page data is maintained as a doubly linked list of smcPage structures
**  within the data area of the cache.  Each entry in the cache contains
**  only the information needed to access another shared memory object of
**  the proper size and type, called a 'Segment'.
**  
**  Pages are marked as free when all registered clients have detached from
**  the page and marked it as "done" (TBD).  The API will allow a client to
**  prune the free pages in order to release space back to the system.
**  
**
******************************************************************************
**/

struct smCache {
    key_t	memKey;		  /* shared mem key			  */
    long    	shmid;		  /* shmid				  */
    void	*cache_addr;	  /* address of cache			  */

    int    	cache_size;	  /* Size of the entire SMC object (bytes)*/
    int		nattached;	  /* # of attached processes		  */
    int 	vm_locked;	  /* Is cache locked from memory paging?  */
    int		fd;		  /* fd of the cache file		  */
    int		new;		  /* is this instance a new cache?	  */
    double  	ctime;            /* time cache was created		  */

    char	dirpath[SZ_PATH]; /* current working directory		  */
    char	froot[SZ_PATH];   /* filename root			  */
    int		seqno;		  /* current working directory		  */

    int    	np_max;		  /* max pages we can manage		  */
    int		np_hiwater;	  /* High water mark of cache (npages)	  */

    XLONG	mem_allocated;	  /* sum of allocated page sizes (bytes)  */
    XLONG	mem_avail;	  /* remaining system memory		  */

    sysConfig_t	sysConfig;	  /* cache config param structs		  */

    smState_t   pstate[MAX_PROCS];/* process state lookup table		  */

    pthread_mutex_t mut;	  /* thread lock			  */

    int 	npages;		  /* No. of pages being managed		  */
    int 	top;		  /* Current top of segment stack	  */
    smcPage_t	pdata[MAX_SEGS];  /* page data (static area)		  */

    int		debug;		  /* debug flag				  */
};




/*  Mapping isn't backed by any file.  MAP_ANON is deprecated but aliased
 *  here for systems that don't have MAP_ANONYMOUS.
 *
 *  (Not currently used)
 */
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

/*
**  Utility Macros
*/
#define align(s,a) 	(((s)+(a)-1) & ~((a)-1))
#define is_true(c)	((strchr("tTyY1\n",c)!=(char *)NULL)?TRUE:FALSE)
#define is_false(c)	((strchr("fFnN0",c)!=(char *)NULL)?TRUE:FALSE)

#define equal_expid(a,b) ((abs(a-b)<0.00864))

#ifndef _dhsUtil_H_
#define min(a,b)	((a<b)?a:b)
#define max(a,b)	((a>b)?a:b)
#endif

#define SEG_SMC(sa)	  ((smCache_t *)smStateGetAddr(sa->pstate))
#define SEG_PTR(sa)	  ((smcSegment_t *)smStateGetAddr(sa->pstate))
#define SEG_ATIME(sa)	  (smStateGetATime(sa->pstate))
#define SEG_LTIME(sa)	  (smStateGetLTime(sa->pstate))
#define SEG_UTIME(sa)	  (smStateGetUTime(sa->pstate))
#define SEG_ADDR(sa)	  (smStateGetAddr(sa->pstate))
#define SEG_READER(sa)	  (smStateGetReader(sa->pstate))
#define SEG_ATTACHED(sa)  (smStateGetAttached(sa->pstate))


/******************************************************************************
 *  Function Prototypes
 *****************************************************************************/


smCache_t *smcOpen  (char *config);
int 	   smcInitialize  (smCache_t *smc);
smCache_t *smcClose  (smCache_t *smc, int free_flag);
smcPage_t *smcGetPage (smCache_t *smc, int type, long size, 
			int attach_flag, int lock_flag);

void 	   smcValidatePage (char *where, smcPage_t *page);
void 	   smcValidateTrap(void);
void 	   smcMutexInit (int id);
void 	   smcMutexOn (void);
void 	   smcMutexOff (void);

void 	   smcSetDir   (smCache_t *smc, char *dirpath);
void 	   smcSetFRoot (smCache_t *smc, char *froot);
void 	   smcSetSeqNo (smCache_t *smc, int seqnum);

char      *smcGetDir   (smCache_t *smc);
char      *smcGetFRoot (smCache_t *smc);
int        smcGetSeqNo (smCache_t *smc);

char      *smcGetPageDir   (smcPage_t *p);
char      *smcGetPageFRoot (smcPage_t *p);
int        smcGetPageSeqNo (smcPage_t *p);
int        smcSetPageSeqNo (smcPage_t *p, int seqno);

void       smcIncrSeqNo (smCache_t *smc);
void       smcDecrSeqNo (smCache_t *smc);
void       smcResetSeqNo (smCache_t *smc);


void      *smcGetPageData (smcPage_t *page);
smcPage_t *smcFindPage (smCache_t *smc, key_t memKey);
smcPage_t *smcNextPage (smCache_t *smc, int timeout);
smcPage_t *smcNextByExpID (smCache_t *smc, double expID);
void       smcPrune  (smCache_t *smc);
void       smcReset  (smCache_t *smc);
void       smcFreePage (smcPage_t *page);

void       smcAttach (smCache_t *smc, smcPage_t *page);
void       smcDetach (smCache_t *smc, smcPage_t *page, int free);
void       smcLock (smcPage_t *page);
void       smcUnlock (smcPage_t *page);


XLONG       smcGetWho (smcPage_t *page);
void        smcSetWho (smcPage_t *page, XLONG who);
double      smcGetExpID (smcPage_t *page);
void        smcSetExpID (smcPage_t *page, double expid);
char       *smcGetObsetID (smcPage_t *page);
void        smcSetObsetID (smcPage_t *page, char *obsetID);
char       *smcGetColID (smcPage_t *page);
void        smcSetColID (smcPage_t *page, char *colID);
int         smcGetExpPageNum (smcPage_t *page);
void        smcSetExpPageNum (smcPage_t *page, int expPageNum);
mdConfig_t *smcGetMDConfig (smcPage_t *page);
void        smcSetMDConfig (smcPage_t *page, mdConfig_t *mdConfig);
fpConfig_t *smcGetFPConfig (smcPage_t *page);
void        smcSetFPConfig (smcPage_t *page, fpConfig_t *fpConfig);

void        smcToggleDebug (smCache_t *smc);
char       *smcType2Str (int type);

void        smcPrintCacheInfo (smCache_t *smc, char *title);
void        smcPrintCfgInfo (sysConfig_t *cfg, char *title);
void        smcPrintPageList (smCache_t *smc, char *title);
void	    smcPrintPageInfo (smcPage_t *page, int pagenum);
void 	    smcPrintSegmentInfo (smcSegment_t *seg, int	segno, char *title);
void	    smcListPages (smCache_t *smc, int npages);
int	    smcEqualExpID (double a, double b);



int  smParseConfig (char *config, sysConfig_t *cfg, int *seg_reader);
int  smParseCfgFile (char *config, sysConfig_t *cfg, int *seg_reader);
int  smParseCfgString (char *config, sysConfig_t *cfg, int *seg_reader); 
void smPrintCfgOpts (sysConfig_t *cfg, char *config, char *title);
/*
static void smGetCfgOption (char **ip, char *keyw, char *val);
static void smSetCfgOption (sysConfig_t *cfg, char *keyw, char *val);
static void smSetConfigDefaults (sysConfig_t *cfg);
*/


void   smStateInit (smState_t *sa);
int    smStateAdd (smState_t *sa, double atime, void *addr, int attached);
int    smStateRemove (smState_t *sa, pid_t rmpid);
int    smStateDefined (smState_t *sa);
void   smStateSetATime (smState_t *sa, double atime);
double smStateGetATime (smState_t *sa);
void   smStateSetLTime (smState_t *sa, double ltime);
double smStateGetLTime (smState_t *sa);
void   smStateSetUTime (smState_t *sa, double utime);
double smStateGetUTime (smState_t *sa);
void   smStateSetReader (smState_t *sa, int reader);
int    smStateGetReader (smState_t *sa);
void   smStateSetAddr (smState_t *sa, void *addr);
void  *smStateGetAddr (smState_t *sa);
void   smStateSetSMC (smState_t *sa, void *smc);
void  *smStateGetSMC (smState_t *sa);
void   smStateSetAttached (smState_t *sa, int attached);
int    smStateGetAttached (smState_t *sa);

/*
static int smStateLookupByPid (smState_t *sa, pid_t pid);
*/
pid_t smPid ();
void  smStateClean (smState_t *sa);
void  smStateDump (smState_t *sa, char *title);

key_t smUtilInitCache (int *new, sysConfig_t *cfg);
key_t smUtilInitSegment (int  *new, char *lockFile);

void  *smUtilAttach (key_t memKey, char *lockfile, XLONG size, int *create);
void   smUtilDetach (key_t memKey, char *lockfile, void *addr, int clearLock);
void   smUtilShmState (key_t memKey);

XLONG  smUtilSize (XLONG *inSize, XLONG  inUnit);
int    smUtilKey2ID (key_t memKey);
int    smUtilExists (key_t memKey, int mode);

double smUtilTime (void);
char  *smUtilTimeStr (double t);
void   smTrap (void);
