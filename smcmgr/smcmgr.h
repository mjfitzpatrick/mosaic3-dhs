/* SMCMGR.H -- Include file definitions for the SMC Manager.
 */


/* Mac OSX doesn't provide the same shmem definitions as linux.
 */

// Hardcode start fo col bias in x direction
#define XBIAS_COL 2066
// note : set to last active column if there is no overscan

#ifndef SZ_PATH
#define SZ_PATH         164
#endif
#ifndef SZ_LINE
#define SZ_LINE         1024
#endif

#ifdef MACOSX
/* ipcs ctl commands
 */
# define SHM_STAT       13
# define SHM_INFO       14

/* shm_mode upper byte flags
 */
#define SHM_DEST       01000   /* segment will be destroyed on last detach  */
#define SHM_LOCKED     02000   /* segment will not be swapped 		    */
#define SHM_HUGETLB    04000   /* segment is mapped via hugetlb 	    */

struct  shminfo {
    unsigned long int shmmax;
    unsigned long int shmmin;
    unsigned long int shmmni;
    unsigned long int shmseg;
    unsigned long int shmall;
    unsigned long int __unused1;
    unsigned long int __unused2;
    unsigned long int __unused3;
    unsigned long int __unused4;
};

struct shm_info {
    int used_ids;
    unsigned long int shm_tot;  	/* total allocated shm 		    */
    unsigned long int shm_rss;  	/* total resident shm 		    */
    unsigned long int shm_swp;  	/* total swapped shm 		    */
    unsigned long int swap_attempts;
    unsigned long int swap_successes;
};
#endif /* MACOSX */


/* Sky-subtraction buffers.
 */

#define  MAX_SKY_FRAMES		4	/* NEWFIRM specific		    */

typedef struct {
    char    colID[SZ_PATH];   		/* collector ID name                */
    int     expPageNum;       		/* page num. w/in this exposure     */
    int     dsize;			/* size of data array (bytes) 	    */
    int    *data;			/* ptr to pixel data 		    */
} skyBuf;


typedef struct {
    double  expID;			/* skyframe exposure ID 	    */
    char    obsSetID[SZ_LINE];          /* skyframe observation ID          */
    skyBuf  frame[MAX_SKY_FRAMES];	/* detector readouts		    */
} skyFrame, *skyFramePtr;


/*  Pixel statistics
 */
typedef struct {
    double  mean, sigma, min, max;      /* display area stats           */
    double  rmean, rsigma, rmin, rmax;  /* reference pixel stats        */
    double  z1, z2;                     /* display scaling              */
    char    *detName;                   /* detector name                */
} pixStat;


typedef struct {
    int         nthread;                /* processing thread number     */
    smCache_t  *smc;			/* SMC struct			*/
    smcPage_t  *page;			/* SMC pixel page		*/
} rtdArg, *rtdArgP;



/**
 *  Prototypes
 */

int  smcProcAll (smCache_t *smc);
int  smcProcNext (smCache_t *smc);
void smcProcess (smCache_t *smc, double expID);
void smcLockExpID (smCache_t *smc, double expID, char *procPages);
void smcUnlockExpID (smCache_t *smc, double expID, char *procPages);
void smcDelExpID (smCache_t *smc, double expID, char *procPages);

void rtdDisplayPixels (void *cdl, smcPage_t *page);
void rtdDisplayThread (void *data);

void smcRectify (smCache_t *smc, double expID);
void smcRectifyPage (smcPage_t *page);

void smMonitor (void);
void smPipeline (int argc, char **argv);
void smUsage (void);
int  smStdFlag (char *opt);
void simMetaData (int nkeyw, int nx, int ny);
void simData (int nx, int ny, int xs, int y2);
void make_raster (int *raster, int nx, int ny, int color);
void make_header (char *hdr, int nkeys);
void make_fits_header (char *hdr, int nkeys);
void processData (smcPage_t *p);
void processMetaData (smcPage_t *p);
void stdHdrOutput (smcPage_t *p);
void smListSegments (smCache_t *smc, int nsegs);
void smListPages (smCache_t *smc, int npages);

int mbusMsgHandler (int fd, void *data);
void myHandler (int from, int subject, char *msg);
void smcPageStats (smCache_t *smc, int npages);
void smcSetDebug (char *msg);
void smcDebugFile (char *file, int value);
void smProcImtype (smCache_t *smc, char *msg);
void smProcImname (smCache_t *smc, char *name);

void rotate (void *iadr, XLONG **oadr, int type, int nx, int ny, int dir);
void flipx (int type, int nx, int ny);
void flipy (int type, int nx, int ny);
void transpose1 (int type, int nx, int ny);
void transpose2 (int type, int nx, int ny);

