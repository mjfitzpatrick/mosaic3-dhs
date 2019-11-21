/*******************************************************************************
 * 
 * dhsUtil.h
 *
 ******************************************************************************/

#ifndef _dhsUtil_H_
 #define _dhsUtil_H_ 1.1.0
 #define _dhsUtil_S_ "1.1.0"
 #define _dhsUtil_A_ "P. N. Daly, N. C. Buchholz"
 #define _dhsUtil_C_ "(c) AURA Inc, 2004. All rights reserved."

 #ifndef XLONG
  #define XLONG	int
 #endif

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
#ifndef DHS_SIMULATION_OK
#define DHS_SIMULATION_OK           2 /* status OK, simulating                */
#endif
#define DHS_ERROR                  -1 /* status bad, abnormal                 */
#define DHS_FALSE                   0 /* boolean false                        */
#define DHS_TRUE         (!DHS_FALSE) /* boolean true                         */
#define DHS_MAXSTR                128 /* maximum length of an obsetID string  */
#define DHS_MAXFIELDS            4096 /* maximum number of fields             */
#define DHS_MAXAVPAIRS           4000 /* maximum number of av-pairs           */

#define DHS_MDTYPE_FITSHEADER       1 /* meta data is a FITS header           */
#define DHS_MDTYPE_AVPAIR           2 /* meta data is an av-pair list         */
#define DHS_MDTYPE_SHIFTLIST        3 /* meta data is a shift list            */
#define DHS_MDTYPE_PHTMTRYDATA      4 /* meta data is photometry data         */
#define DHS_MDTYPE_ARRAYDATA        5 /* meta data is an array                */

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
#define UNKNOWN			0     /
#define READS_UP		1     /* Direction of increasing pxl number with time */
#define READS_DN		-1
#define READS_RGT		2
#define READS_LFT		-2

 /******************************************************************************
 * typedef(s)
 ******************************************************************************/
 typedef XLONG dhsHandle;

 typedef struct fpConfig_str {  /* focal plane configuration structure        */
  XLONG xSize;   /* size of a row on the focal plane (number of columns)      */
  XLONG ySize;   /* size of a column on the focal plane (number of rows)      */
  XLONG xStart;  /* column index of the first pixel in this part of plane     */
  XLONG yStart;  /* row index of the first pixel in this part of plane        */
  XLONG dataType; /* data type of the pixels (number of bytes)                */
  XLONG xDir;    /* direction of increasing (later time) column number        */
  XLONG yDir;    /* data sent from monsoon places the first pixel received    */
                 /* from the DHE as pixel 1,1 in the buffer and assume it to  */
                 /* be in the lower left corner of the detector               */
  XLONG xDetSz;  /* size of a single detector row (Number of pixel columns)   */
  XLONG yDetSz;  /* size of a single detector column (Number of pixel rows)   */
  XLONG xDetCnt; /* size of a mosaic row number of columns of detectors       */
  XLONG yDetCnt; /* size of a mosaic column number of rows of detectors       */
  XLONG xAmpCnt; /* number of readout amps per row of a detector              */
  XLONG yAmpCnt; /* number of readout amps per col of a detector              */
 } fpConfig_t, *fpConfig_p, **fpConfig_s;

 typedef struct mdConfig_str {    /* meta data configuration structure        */
  XLONG metaType;                 /* conceptual type of the meta data         */
  XLONG numFields;                /* number of fields in the meta data array  */
  XLONG fieldSize[DHS_MAXFIELDS]; /* number of items in the field             */
  XLONG dataType[DHS_MAXFIELDS];  /* data type of the values in the field     */

 } mdConfig_t, *mdConfig_p, **mdConfig_s;

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
 void dhsUtilSysOpen       ( XLONG *istat, char *resp, dhsHandle *dhsID, XLONG whoami );
 void dhsUtilSysClose      ( XLONG *istat, char *resp, dhsHandle dhsID );
 void dhsUtilOpenConnect   ( XLONG *istat, char *resp, dhsHandle *dhsID, XLONG whoami, fpConfig_t *cfg );
 void dhsUtilCloseConnect  ( XLONG *istat, char *resp, dhsHandle dhsID );
 void dhsUtilOpenExp       ( XLONG *istat, char *resp, dhsHandle dhsID, fpConfig_t *cfg, double *expID, char *obsetID );
 void dhsUtilCloseExp      ( XLONG *istat, char *resp, dhsHandle dhsID, double expID );
 void dhsUtilSendMetaData  ( XLONG *istat, char *resp, dhsHandle dhsID, void *blkAddr, size_t blkSize, mdConfig_t *cfg, double *expID, char *obsetID );
 void dhsUtilSendPixelData ( XLONG *istat, char *resp, dhsHandle dhsID, void *pxlAddr, size_t pxlSize, fpConfig_t *cfg, double *expID, char *obsetID );
 void dhsUtilReadImage     ( XLONG *istat, char *resp, dhsHandle dhsID );
 void dhsUtilIOCtl         ( XLONG *istat, char *resp, dhsHandle dhsID, XLONG ioctlFunction, double *expID, char *obsetID, ... );
#endif

extern  char __simStr[10];
