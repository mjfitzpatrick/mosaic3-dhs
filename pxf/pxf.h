/******************************************************************************
 * define(s)
 ******************************************************************************/
/******************************************************************************
 * typedef(s)
 ******************************************************************************/

extern char pxfDIR[256];
extern char pxfFILENAME[256];
extern int  pxfFLAG;
extern int  pxfFileCreated;
extern int  pxfExtChar;
extern int  procDebug;


/* Keyword monitoring structs.
*/
#define	SZ_KEYWLIST		100

char	*keywList[SZ_KEYWLIST];		/* list of keywords to monitor	*/
int	NKeywords;			/* No. of keywords monitored	*/




/*******************************************************************************
 * prototype(s)
 ******************************************************************************/

int  mbusMsgHandler (int fd, void *data);
void myHandler (int from, int subject, char *msg);

void procDCAMetaData (smcPage_t *p);
void procDCAData (smcPage_t *p);
int  inKeywList (char *kwdb, char *keyw);

void procFITSData (smcPage_t *p, fitsfile *fd);
void procFITSMetaData (smcPage_t *p, fitsfile *fd);

void pxfFileOpen(XLONG *istat, char *resp, double *expID,
		 smCache_t *smc, fitsfile **fd);

void pxfSendMetaData(XLONG *istat, char *resp, void *blkAddr, size_t blkSize,
		     mdConfig_t * cfg, double *expID, char *obsetID,
		     fitsfile *fitsID);
void pxfSendPixelData(XLONG *istat, char *resp, void *pxlAddr, size_t blkSize,
		      fpConfig_t * cfg, double *expID, char *obsetID,
		      fitsfile * fitsID);
void pxfFileOpen (XLONG *istat, char *resp, double *expID,
		  smCache_t * smc, fitsfile ** fd);

void pxfProcess (smCache_t *smc, double expID);
