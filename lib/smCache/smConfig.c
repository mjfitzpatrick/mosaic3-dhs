/*
 *  SMCONFIG --  Read the Shared Memory Cache config string/file and set
 *  parameters appropriately.  These values will be overwritten in the event
 *  we later attach to an existing smCache.
 *
 *	 stat = smParseConfig (config)			// Public procedures
 *	stat = smParseCfgFile (config)
 *    stat = smParseCfgString (config)
 *
 *	       smGetCfgOption (ip, keyw, val)		// Private procedures
 *	       smSetCfgOption (keyw, val)
 *	       smPrintCfgOpts (cfg)
 *
 *	             smCfg_UT ()			// Unit Test Procedure
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stddef.h>

#ifdef MACOSX
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include "smCache.h"


/*  Private procedures.
 */
static void  smGetCfgOption (char **ip, char *keyw, char *val);
static void  smSetCfgOption (sysConfig_t *cfg, char *keyw, char *val);
static void  smSetConfigDefaults (sysConfig_t *cfg);


#ifdef SM_UNIT_TEST
static void smCfg_UT ();

main (int argc, char **argv) { 
    smCache_t *smc = smcOpen (NULL); 
    smCfg_UT (); 
    smcClose (smc); 
}
#endif



/* SMPARSECONFIG -- Parse a configuration specification from a string or a
 * filename.
 */

#define MB              (1024*1024)
#define KB              1024

int
smParseConfig (char *config, sysConfig_t *cfg, int *seg_reader)
{
    int		pgsize, status=OK;


    /*  Initialize the cache file and defaults in case there are missing
     *  config params. 
     */
    memset (cfg->cache_path,  0, SZ_PATH);
    sprintf (cfg->cache_path,  DEF_CACHE_FILE, (int) getuid());
    smSetConfigDefaults (cfg);

    /* Set the local configuration option defaults.
     */
    *seg_reader = TY_ANY;

    /* Initialize the system resource parameter struct.  We use this
     * when determining the resource limits on the machine.
     */
    cfg->sys_page_size    = getpagesize () / KB;
#ifdef LINUX
    cfg->sys_phys_pages   = sysconf (_SC_PHYS_PAGES) / cfg->sys_page_size;
    cfg->sys_avphys_pages = sysconf (_SC_AVPHYS_PAGES) / cfg->sys_page_size;
#endif
#ifdef MACOSX
    { 	int mib[2], physmem, usermem;
	size_t len;

	mib[0] = CTL_HW;
	mib[1] = HW_PHYSMEM;
	len - sizeof (physmem);
	sysctl(mib, 2, &physmem, &len, NULL, 0);

	mib[1] = HW_USERMEM;
	len - sizeof (usermem);
	sysctl(mib, 2, &usermem, &len, NULL, 0);

	cfg->sys_phys_pages   = physmem / cfg->sys_page_size;
	cfg->sys_avphys_pages = usermem / cfg->sys_page_size;
    }
#endif

    cfg->sys_pmask        = cfg->sys_page_size - 1;
    cfg->debug            = SMC_DEBUG;


    if (config != (char *) NULL) {
        /*  First see if we have a filename of config params, else parse
         *  the string directly. 
         */
        if (access (config, R_OK) == 0)
            status = smParseCfgFile (config, cfg, seg_reader);
        else {
            status = smParseCfgString (config, cfg, seg_reader);

	    /*  If the file specified by the config string exists, we're
	     *  attaching to an existing cache.  Read the config file
	     *  for the parameters.
	     */
	    if (access (cfg->cache_path, R_OK|W_OK) == 0) {
    		if (SMC_DEBUG) 
		    fprintf (stderr, "Re-reading config '%s'\n",
			cfg->cache_path);
		smSetConfigDefaults (cfg);
                status = smParseCfgFile (cfg->cache_path, cfg, seg_reader);
	    }
        }
    }

    /*  Compute a page-aligned cache size large enough to hold the runtime
     *  struct and the page array.
     */
    pgsize = cfg->sys_page_size;
    cfg->cache_size = ((sizeof(smCache_t) + pgsize) / pgsize) * pgsize;

    return (status);
}


/* SMPARSECONFIGFILE -- Parse a configuration specification from a file.
 */
int
smParseCfgFile (char *config, sysConfig_t *cfg, int *seg_reader)
{
    char  *ip, keyw[SZ_KEYW], val[SZ_VAL], line[SZ_LINE], *nread;
    int   len, done=0;
    FILE  *fd;
    char  *fgets();


    if (config == (char *)NULL)
	return (ERR);				/* shouldn't be here */

    /* Open the file for reading. */
    if ((fd = fopen (config, "r")) == (FILE *)NULL) {
	fprintf (stderr, "Cannot open file '%s'\n", config);
	return (ERR);
    }


    /* Now loop through the file one line at a time for keyw=val pairs. */
    memset (line, 0, SZ_LINE);
    while ((nread = fgets ((char *)line, SZ_LINE, fd))) {

	/* Skip blank lines and comments. */
	if (done)
	   break;
	if (line[0] == '\n' || line[0] == '#')
	   continue;

        for (ip=line; *ip && (ip-line) <= len; ) {
	    smGetCfgOption (&ip, keyw, val);

	    /* Break when we reach the End_Of_Config in a lockfile. */
	    if (strncmp (keyw, "E_O_C", 5) == 0) {
		done++;
		break;
	    }

	    /* Process the local options.
	     */
	    if (strncmp (keyw, "segments", 3) == 0) {
	        if (val[0] ==  'a')		/* all segments		*/
		    *seg_reader = TY_ANY;
	        if (val[0] ==  'n')		/* no segments		*/
		    *seg_reader = TY_NONE;
	        if (val[0] ==  'd')		/* data only		*/
		    *seg_reader = TY_DATA;
	        if (val[0] ==  'm')		/* metadata only	*/
		    *seg_reader = TY_META;
	        else
        	    fprintf (stderr, 
		        "Warning: Unrecognized option '%s'='%s'\n", keyw, val);
	    } else
	        smSetCfgOption (cfg, keyw, val);
	}

	memset (line, 0, SZ_LINE);
    }

    /* Update the filename in the config struct. 
     */
    memset (cfg->config, 0, SZ_PATH);
    memset (cfg->cache_path, 0, SZ_PATH);

    sprintf (cfg->config, "%s", config);
    sprintf (cfg->cache_path, "%s", config);

    return (OK);
}


/* SMPARSECONFIGSTRING -- Parse a configuration specification from a string.
 */
int
smParseCfgString (char *config, sysConfig_t *cfg, int *seg_reader)
{
    char  *ip, keyw[SZ_KEYW], val[SZ_VAL];
    register int len=0;


    if (config == (char *)NULL)
	return (ERR);				/* shouldn't be here 	*/

    len = strlen (config);
    for (ip=config; *ip && (ip-config) <= len; ) {
	smGetCfgOption (&ip, keyw, val);
    
	/* Process the local options.
	 */
	if (strncmp (keyw, "segments", 3) == 0) {
	    if (val[0] ==  'a')			/* all segments		*/
		*seg_reader = TY_ANY;
	    if (val[0] ==  'n')			/* no segments		*/
		*seg_reader = TY_NONE;
	    if (val[0] ==  'd')			/* data only		*/
		*seg_reader = TY_DATA;
	    if (val[0] ==  'm')			/* metadata only	*/
		*seg_reader = TY_META;
	    else
        	fprintf (stderr, 
		    "Warning: Unrecognized option '%s'='%s'\n", keyw, val);
	} else
	    smSetCfgOption (cfg, keyw, val);
    }

    memset (cfg->config, 0, SZ_PATH);
    sprintf (cfg->config, "%s", config);

    return (OK);
}


/* SMGETCONFIGOPTION -- Get the config parameter based on the keyword.
 */
static void
smGetCfgOption (char **ip, char *keyw, char *val)
{
    register int i;
    char *op = *ip;


    memset (keyw, 0, SZ_KEYW);
    memset (val,  0, SZ_KEYW);

    /* Extract the keyword/value pair strings. */
    for (i=0; (*op && *op != '=' && (isalnum(*op) || *op == '_')); )
        keyw[i++] = *op++;
    while (*op == '=' || isspace(*op)) op++;

    for (i=0; (*op && *op != ',' && !isspace(*op)); )
        val[i++] = *op++;
    while (*op == ',' || isspace(*op)) op++;

    *ip = op;
}


/* SMSETCONFIGOPTION -- Set the config parameter based on the keyword.
 */
static void
smSetCfgOption (sysConfig_t *cfg, char *keyw, char *val)
{
    if (strncmp (keyw, "cache_path", 7) == 0 ||
	strncmp (keyw, "cache_file", 7) == 0) {
	    if (strlen (val) > 0) {
	        memset (cfg->cache_path, 0, SZ_PATH);
                strncpy (cfg->cache_path, val, strlen(val));
	    }

    } else if (strncmp (keyw, "cache_size", 7) == 0) {
        cfg->cache_size = atoi (val);

    } else if (strncmp (keyw, "cache_memKey", 7) == 0) {
        cfg->cache_memKey = atoi (val);

    } else if (strncmp (keyw, "cache_addr", 7) == 0) {
#ifdef USE_32BIT
        cfg->cache_addr = (void *)atoi (val);
#else
        cfg->cache_addr = (void *)((long)atoi (val));
#endif

    } else if (strncmp (keyw, "nsegs", 4) == 0) {
        cfg->nsegs = min (MAX_SEGS, atoi(val));

    } else if (strncmp (keyw, "debug", 4) == 0) {
        cfg->debug = is_true (val[0]);
	if (cfg->smc)
            cfg->smc->debug = is_true (val[0]);

    } else if (strncmp (keyw, "throttle_time", 10) == 0) {
        cfg->throttle_time = atoi (val);

    } else if (strncmp (keyw, "throttle_ntry", 10) == 0) {
        cfg->throttle_ntry = atoi (val);

    } else if (strncmp (keyw, "min_seg_size", 9) == 0) {
	int sps = cfg->sys_page_size;
        cfg->min_seg_size = max(sps, atoi(val));

    } else if (strncmp (keyw, "verbose", 4) == 0) {
        cfg->verbose = (strchr ("tTyY1", val[0]) != (char *)NULL);

    } else if (strncmp (keyw, "lock_cache", 6) == 0) {
        cfg->lock_cache = (strchr ("tTyY1", val[0]) != (char *)NULL);

    } else if (strncmp (keyw, "lock_segs", 6) == 0)
        cfg->lock_segs = (strchr ("tTyY1", val[0]) != (char *)NULL);

    /* No-op params so we can read a lockfile or pass back local opts.
     */
    else if (strncmp (keyw, "pid", 3) == 0) ;
    else if (strncmp (keyw, "mFd", 3) == 0) ;
    else if (strncmp (keyw, "size", 4) == 0) ;
    else if (strncmp (keyw, "segments", 3) == 0) ;

    else
        fprintf (stderr, "Warning: Unrecognized option '%s'='%s'\n", keyw, val);
}


/* SMPRINTCONFIGOPTS -- Print the config options.
 */
void 
smPrintCfgOpts (sysConfig_t *cfg, char *config, char *title)
{
    if (cfg == (sysConfig_t *)NULL) {
	fprintf (stderr, "smPrintCfgOpts: NULL cache pointer.\n");
	return;
    }

    fprintf (stderr,"%s\n", (title ? title : ""));
    fprintf (stderr,"        config = '%s'\n", (config ? config : "NULL"));
    fprintf (stderr,"    cache_path = '%s'\n", cfg->cache_path);
    fprintf (stderr,"    cache_addr = 0x%x\n", (int) cfg->cache_addr);
    fprintf (stderr,"    cache_size = %d bytes\n", cfg->cache_size);
    fprintf (stderr,"         nsegs = %d\n", cfg->nsegs);
    fprintf (stderr,"  min_seg_size = %d (bytes)\n", cfg->min_seg_size);
    fprintf (stderr," throttle_time = %d\n", cfg->throttle_time);
    fprintf (stderr," throttle_ntry = %d\n", cfg->throttle_ntry);
    fprintf (stderr,"    lock_cache = %d\n", cfg->lock_cache);
    fprintf (stderr,"     lock_segs = %d\n", cfg->lock_segs);
    fprintf (stderr,"       verbose = %d\n", cfg->verbose);
    fprintf (stderr,"\n");
}


/* SMSETCONFIGDEFAULTS -- Set the default values for a sysConfig struct.
 */
static void
smSetConfigDefaults (cfg)
sysConfig_t *cfg;
{
    cfg->cache_size	= 0;			/* force a computation */
    cfg->max_segs	= MAX_SEGS;
    cfg->nsegs		= MAX_SEGS;
    cfg->min_seg_size	= DEF_MIN_SEG_SIZE;
    cfg->lock_cache	= DEF_LOCK_CACHE;
    cfg->lock_segs	= DEF_LOCK_SEGS;
    cfg->throttle_time	= DEF_THROTTLE_TIME;
    cfg->throttle_ntry  = DEF_THROTTLE_NTRY;
    cfg->interval       = DEF_POLL_INTERVAL;
    cfg->verbose        = DEF_VERBOSE;
}


#ifdef SM_UNIT_TEST

/* SMCONFIG_UT -- Unit test module for the smCache config parsing.
 */
static void 
smCfg_UT ()
{
    int  stat  = 0;
    char *cfg1 = "/tmp/cfgtest";
    char *cfg2 = "cache_file=/tmp/foo, nsegs=12,min_seg=4096 lock_cache=false";
    FILE *fd;


    printf ("Testing NULL Cfg:\n");
    	stat = smParseCfgString ((char *)NULL);
        if (SMC_DEBUG) 
	    smPrintCfgOpts((sysConfig_t *)NULL, (char *)NULL, (char *)NULL);
    	printf ("Status: %d\n\n\n", stat)

    printf ("Testing File Cfg:\n");

	fd = fopen (cfg1, "w+");
	fprintf (fd, "\n");
	fprintf (fd, "cache_file=/tmp/foo\n");
	fprintf (fd, "# Comment test\n");
	fprintf (fd, "npages =  12\n");
	fprintf (fd, "page_sz = 4096 \n");
	fprintf (fd, "lock_cache=false\n");
	fclose  (fd);

    	stat = smParseCfgFile (cfg1);
    	if (SMC_DEBUG) smPrintCfgOpts (cfg1, (char *)NULL, (char *)NULL);
    	printf ("Status: %d\n", stat);

    printf ("Testing String Cfg:\n");
    	stat = smParseCfgString (cfg2);
    	if (SMC_DEBUG) smPrintCfgOpts (cfg2, (char *)NULL, (char *)NULL);
    	printf ("Status: %d\n\n\n", stat);

}
#endif
