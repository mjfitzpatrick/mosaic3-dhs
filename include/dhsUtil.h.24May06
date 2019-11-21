/*******************************************************************************
 * 
 * __doc__ \section {The dhsUtil <<VERSION>> Library}
 * __doc__ \subsection {dhsUtil.h}
 * __doc__ \begin{description}
 * __doc__  \item[\sc use:] \emph{\#include ``dhsUtil.h''}
 * __doc__  \item[\sc description:] this file contains all common code
 * __doc__    required by the functions needed to build the static
 * __doc__    and dynamic dhsUtil libraries. These libraries
 * __doc__    abstract the DHS interface to the system.
 * __doc__  \item[\sc argument(s):]  not applicable
 * __doc__  \item[\sc return(s):] not applicable
 * __doc__  \item[\sc last modified:] Monday, 4 November 2002
 * __doc__ \end{description} 
 *
 ******************************************************************************/

#ifndef _dhsUtil_H_
#define _dhsUtil_H_ 1.0.2
#define _dhsUtil_S_ "1.0.2"
#define _dhsUtil_A_ "P. N. Daly, N. C. Buchholz"
#define _dhsUtil_C_ "(c) AURA Inc, 2004. All rights reserved."

/*******************************************************************************
  * include(s)
  ******************************************************************************/
#ifndef _STDIO_H
 #include <stdio.h>
#endif
 
#ifndef _STDLIB_H
 #include <stdlib.h>
#endif
 
#ifndef _STDARG_H
 #include <stdarg.h>
#endif
 
#ifndef _UNISTD_H
 #include <unistd.h>
#endif
 
#ifndef _FCNTL_H
 #include <fcntl.h>
#endif
 
#ifndef _SYS_TYPES_H
 #include <sys/types.h>
#endif
 
#ifndef _CTYPE_H
 #include <ctype.h>
#endif

#ifndef _STRING_H
 #include <string.h>
#endif
 
#ifndef _SYS_IPC_H
 #include <sys/ipc.h>
#endif
 
#ifndef _SYS_SHM_H
 #include <sys/shm.h>
#endif
 
#ifndef _SYS_STAT_H
 #include <sys/stat.h>
#endif
 
#ifndef _MNSN_STD_HDR_
 #include <mnsnStd.h>
#endif
 
#ifndef __MNSNERR__
 #include <mnsnErrNo.h>
#endif

#ifndef DEBUG
 #define DEBUG 1
#endif
#include <debug.h>
 
#ifndef _shmUtil_H_
#include "shmUtil.h"
#endif
 
/******************************************************************************
  * define(s)
  ******************************************************************************/
#define DHS_OK                      0 /* status OK, normal                    */
#define DHS_SIMULATION_OK           2 /* status OK, simulating                */
#define DHS_ERROR                  -1 /* status bad, abnormal                 */
#define DHS_FALSE                   0 /* boolean false                        */
#define DHS_TRUE         (!DHS_FALSE) /* boolean true                         */
#define DHS_MAXSTR                128 /* maximum length of an obsetID string  */
#define DHS_MAXFIELDS            4096 /* maximum number of fields             */
#define DHS_MAXAVPAIRS           4000 /* maximum number of av-pairs           */
#define DHS_MDTYPE_FITSHEADER       1 /* meta data is a FITS header           */
#define DHS_MDTYPE_AVPAIR           2 /* meta data is an av-pair list         */
#define DHS_MDTYPE_SHIFTLIST        3 /* meta data is a shift list            */
#define DHS_MDTYPE_ARRAYDATA        4 /* meta data is an array                */
#define DHS_FITS_NUMFIELDS          3 /* number of fields for FITS            */
#define DHS_FITS_SEP1               2 /* first separator "= "                 */ 
#define DHS_FITS_SEP2               3 /* second separator " / "               */ 
#define DHS_FITS_NAMESIZE           8 /* name size for FITS                   */ 
#define DHS_FITS_VALSIZE           20 /* value size for FITS                  */
#define DHS_FITS_COMMENT           46 /* comment size for FITS                */
#define DHS_FITS_FINAL              1 /* '\0' at end of record                */
#define DHS_FITS_RECLEN            80 /* 8 + 2 + 20 + 3 + 46 + 1              */
#define DHS_FITS_RAWLEN            74 /* 8 + 20 + 46                          */
#define DHS_AVP_NUMFIELDS           3 /* number of fields for av-pairs        */
#define DHS_AVP_NAMESIZE           32 /* name size for av-pairs               */
#define DHS_AVP_VALSIZE            32 /* value size for av-pairs              */
#define DHS_AVP_COMMENT            64 /* comment size for av-pairs            */
#define DHS_AVP_RAWLEN            128 /* 32 + 32 + 64                         */
#define DHS_AVP_NAMEFMT        "%32s" /* format name                          */
#define DHS_AVP_COMMENTFMT     "%64s" /* format comment                       */
#define DHS_AVP_VALDFMT   "%+32.16lf" /* format value as double               */
#define DHS_AVP_VALFFMT    "%+32.16f" /* format value as float                */
#define DHS_AVP_VALIFMT       "%+32d" /* format value as integer              */
#define DHS_AVP_VALLFMT      "%+32ld" /* format value as long                 */
#define DHS_AVP_VALSFMT        "%32s" /* format value as string               */
#define DHS_LIST_NELMS             64 /* lists have 64 elements ...           */
#define DHS_LIST_BSIZE              2 /*  ... of 2 bytes each                 */
#define DHS_LIST_RAWLEN             2 /* same as bsize                        */
#define DHS_UBYTE                   1 /* 1 byte,  [0,255]                     */
#define DHS_BYTE                   -1 /* 1 byte,  [-128,127]                  */
#define DHS_USHORT                  2 /* 2 bytes, [0,65535]                   */
#define DHS_SHORT                  -2 /* 2 bytes, [-32768,32767]              */
#define DHS_UINT                    4 /* 4 bytes, [0,4294967295]              */
#define DHS_INT                    -4 /* 4 bytes, [-2147483648,-2147483648]   */
#define DHS_ULONG                   8 /* 8 bytes, [0,(2^64)-1]                */
#define DHS_LONG                    8 /* 8 bytes, [-1*(2^63),(2^63)-1]        */
#define DHS_FLOAT                  32 /* 4 bytes, single precision            */
#define DHS_DOUBLE                 64 /* 8 bytes, double precision            */
#define DHS_IAMOCS             0xFACE /* ID for observation control system    */
#define DHS_IAMMSL             0xCAFE /* ID for monsoon supervisor layer      */
#define DHS_IAMPAN             0xABCD /* ID for pixel acquisition node        */
#define DHS_IAMWHO             0xFEED /* ID yet to be established             */
#define DHS_IOC_OBSCONFIG      0x0100 /* ioctl to define an obsetID set       */
#define DHS_IOC_MDCONFIG       0x0200 /* ioctl to define mdConfig set         */
#define DHS_IOC_FPCONFIG       0x0400 /* ioctl to define fpConfig set         */
#define DHS_IOC_KEYWORD_TRANS  0x0800 /* ioctl to define keyword translation  */
#define DHS_IOC_DEBUG_LVL      0x1000 /* iotcl to define debug level          */
#define DHS_IOC_SIMULATION     0x2000 /* ioctl to define simulation level     */
#define DHS_IOC_SETFILENAME    0x4000 /* ioctl to set file name               */
#define DHS_IOC_SETDIR         0x8000 /* ioctl to set directory name          */

 /******************************************************************************
  * typedef(s)
  ******************************************************************************/
 typedef long dhsHandle;

 typedef struct fpConfig_str { /* focal plane configuration structure          */
   ulong xSize;   /* size of a row on the focal plane (number of columns)      */
   ulong ySize;   /* size of a column on the focal plane (number of rows)      */
   ulong xStart;  /* column index of the first pixel in this part of plane     */
   ulong yStart;  /* row index of the first pixel in this part of plane        */
   long dataType; /* data type of the pixels (number of bytes)                 */
 } fpConfig_t, *fpConfig_p, **fpConfig_s;

 typedef struct mdConfig_str { /* meta data configuration structure            */
   ulong metaType;                 /* conceptual type of the meta data         */
   ulong numFields;                /* number of fields in the meta data array  */
   ulong fieldSize[DHS_MAXFIELDS]; /* number of items in the field             */
   ulong dataType[DHS_MAXFIELDS];  /* data type of the values in the field     */
 } mdConfig_t, *mdConfig_p, **mdConfig_s;

 char __simStr[10];

 /*******************************************************************************
  * prototype(s)
  ******************************************************************************/
 #define dhsSysOpen        dhsUtilSysOpen       
 #define dhsSysClose       dhsUtilSysClose      
 #define dhsOpenConnect    dhsUtilOpenConnect   
 #define dhsCloseConnect   dhsUtilCloseConnect  
 #define dhsOpenExp        dhsUtilOpenExp       
 #define dhsCloseExp       dhsUtilCloseExp      
 #define dhsSendMetaData   dhsUtilSendMetaData  
 #define dhsSendPixelData  dhsUtilSendPixelData 
 #define dhsReadImage      dhsUtilReadImage     
 #define dhsIOCtl          dhsUtilIOCtl         
 void dhsUtilSysOpen       ( long *istat, char *resp, dhsHandle *dhsID, ulong whoami );
 void dhsUtilSysClose      ( long *istat, char *resp, dhsHandle dhsID );
 void dhsUtilOpenConnect   ( long *istat, char *resp, dhsHandle *dhsID, ulong whoami, fpConfig_t *cfg );
 void dhsUtilCloseConnect  ( long *istat, char *resp, dhsHandle dhsID );
 void dhsUtilOpenExp       ( long *istat, char *resp, dhsHandle dhsID, fpConfig_t *cfg, double *expID, char *obsetID );
 void dhsUtilCloseExp      ( long *istat, char *resp, dhsHandle dhsID, double expID );
 void dhsUtilSendMetaData  ( long *istat, char *resp, dhsHandle dhsID, void *blkAddr, size_t blkSize, mdConfig_t *cfg, double *expID, char *obsetID );
 void dhsUtilSendPixelData ( long *istat, char *resp, dhsHandle dhsID, void *pxlAddr, size_t pxlSize, fpConfig_t *cfg, double *expID, char *obsetID );
 void dhsUtilReadImage     ( long *istat, char *resp, dhsHandle dhsID );
 void dhsUtilIOCtl         ( long *istat, char *resp, dhsHandle dhsID, ulong ioctlFunction, double *expID, char *obsetID, ... );
#endif
