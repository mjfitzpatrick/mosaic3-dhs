/*******************************************************************************
 * include(s):
 *******************************************************************************/
#if !defined(_dhsTcl_H_)
 #include "dhsTcl.h"
#endif

#if !defined(_dhsImpl_H_)
 #include "dhsImplementationSpecifics.h"
#endif

/*******************************************************************************
 * global(s):
 *******************************************************************************/
#define DHS_FALSE        0
#define DHS_TRUE         (!DHS_FALSE)
#define DHS_NAME_LEN     DHS_IMPL_MAXSTR
#define DHS_RESULT_LEN   DHS_IMPL_MAXSTR
#define DHS_ITEM_LEN     DHS_IMPL_MAXSTR
#define DHS_EXAMPLE_LEN  DHS_IMPL_MAXSTR
#define DHS_HELP_LEN     MAXMSG

#ifdef TCL83
 #define TCL_STUB_VERSION "8.3"
#endif
#ifdef TCL84
 #define TCL_STUB_VERSION "8.4"
#endif

/*******************************************************************************
 * shut the compiler up!
 *******************************************************************************/
#if defined(__PTIME__)
 #include <time.h>
 #include <stdio.h>
 char *pTime ( char *S ) {
  struct tm *tim;
  time_t t = time((time_t *)&t);
  tim = localtime((time_t *) &t);
  if ( S == (char *)NULL ) return S;
  (void) sprintf(S,"%4d%02d%02d:%02d%02d%02d-", tim->tm_year+1900, tim->tm_mon+1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);
  return S;
 }
#endif

/*******************************************************************************
 * static(s):
 *******************************************************************************/
static char response[MAXMSG];
static char result[DHS_RESULT_LEN];

/*******************************************************************************
 * variable(s):
 *******************************************************************************/
char __simStr[10] = "";
char resp2[MAXMSG] = "";
int dhsDebug = 0;

/*******************************************************************************
 * structure(s)
 *******************************************************************************/
typedef struct help_str { char item[DHS_NAME_LEN]; } help_t, *help_p, **help_s;

static help_t NM[] =  {
 {"                  "},
 {"DHS               "},
 {"                  "},
 {"Command           "},
 {"------------------"},
 {"                  "},
 {"dhs::help         "},
 {"dhs::version      "},
 {"dhs::SysOpen      "},
 {"dhs::SysClose     "},
 {"dhs::OpenConnect  "},
 {"dhs::CloseConnect "},
 {"dhs::OpenExp      "},
 {"dhs::CloseExp     "},
 {"dhs::IOctl        "},
 {"dhs::ReadImage    "},
 {"dhs::PixelData    "},
 {"dhs::MetaData     "}};
#define NM_NUM (sizeof(NM)/sizeof(help_t))
static help_t HP[] =  {
 {"                            "},
 {"MONSOON Data Handling System"},
 {"                            "},
 {"Description                 "},
 {"----------------------------"},
 {"                            "},
 {"Report help                 "},
 {"Report version              "},
 {"Open DHS system             "},
 {"Close DHS system            "},
 {"Open DHS connection         "},
 {"Close DHS connection        "},
 {"Open DHS exposure           "},
 {"Close DHS exposure          "},
 {"Perform IOCTL function      "},
 {"Read an image               "},
 {"Send an image               "},
 {"Send meta-data block        "}};
#define HP_NUM (sizeof(HP)/sizeof(help_t))
static help_t EG[] =  {
 {"                                                                           "},
 {_dhsTcl_S_},
 {"                                                                           "},
 {"Example                                                                    "},
 {"---------------------------------------------------------------------------"},
 {"                                                                           "},
 {"dhs::help                                                                  "},
 {"dhs::version                                                               "},
 {"set sID     [dhs::SysOpen <sysID>]                                         "},
 {"set dhsStat [dhs::SysClose <sID>]                                          "},
 {"set cID     [dhs::OpenConnect <subID> {cfgList}]                           "},
 {"set dhsStat [dhs::CloseConnect <cID>]                                      "},
 {"set eID     [dhs::OpenExp <cID> <{cfgList}> <expID> <obsID>]               "},
 {"set dhsStat [dhs::CloseExp <eID> <expID>]                                  "},
 {"set dhsStat [dhs::IOctl <eID> <ioctl> <expID> <obsID>]                     "},
 {"set dhsStat [dhs::ReadImage <eID>]                                         "},
 {"set dhsStat [dhs::PixelData <eID> {data} <nelms> {cfgList} <expID> <obsID>]"},
 {"set dhsStat [dhs::MetaData <eID> {data} <nelms> {cfgList} <expID> <obsID>] "}};
#define EG_NUM (sizeof(EG)/sizeof(help_t))

/*******************************************************************************
 * dhsHelpTcl ( ... )
 *  Use: dhs::help
 *******************************************************************************/
static int dhsHelpTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 int ic=0, nc=0, room=DHS_TRUE;
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* write out everything else */
 for ( ic=0; ic<NM_NUM; ic++ ) {
  if ( ! (room=( (DHS_HELP_LEN-nc) > (DHS_NAME_LEN+(DHS_ITEM_LEN*2)) ? DHS_TRUE : DHS_FALSE )) ) break;
  nc += sprintf((char *)&response[nc],"%s %s %s\n",NM[ic].item,HP[ic].item,EG[ic].item);
 }
 /* set result and return */
 (void) Tcl_SetResult(interp,response,TCL_STATIC);
 return TCL_OK;
}

/*******************************************************************************
 * dhsVersionTcl ( ... )
 *  Use: dhs::version
 *******************************************************************************/
static int dhsVersionTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 int nc=0;
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* write */
 nc = sprintf(response,"\nDHS Package v%s by %s.\n%s\nThis release: %s\n",_dhsTcl_S_,_dhsTcl_A_,_dhsTcl_C_,_dhsTcl_D_);
 /* set result and return */
 (void) Tcl_SetResult(interp,response,TCL_STATIC);
 return TCL_OK;
}

/*******************************************************************************
 * dhsReadImageTcl ( ... )
 *  Use: set dhsStat [dhs::ReadImage <eID>]
 *******************************************************************************/
static int dhsReadImageTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 dhsHandle eID=(dhsHandle)0;
 int ival=0;
 long lstat=0;
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* check handle */
 if ( Tcl_GetInt(interp,argv[1],&ival) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsReadImageTcl-E-bad handle\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 eID = (dhsHandle)ival;
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::ReadImage: eID=%d\n",(int)eID); (void) fflush(stderr);
 #endif
 /* execute the dhs function */
 dhsReadImage(&lstat,response,eID);
 if ( STATUS_BAD(lstat) ) {
  (void) Tcl_SetResult(interp,response,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::ReadImage: lstat=%ld\n",lstat); (void) fflush(stderr);
 #endif
 /* return result */  
 (void) sprintf(result,"%ld",lstat);
 (void) Tcl_SetResult(interp,result,TCL_STATIC);
 return TCL_OK;
}

/*******************************************************************************
 * dhsIOctlTcl ( ... )
 *  Use: set dhsStat [dhs::IOctl <eID> <ioctl> <expID> <obsID>]
 *******************************************************************************/
static int dhsIOctlTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 dhsHandle eID=(dhsHandle)0;
 double expID=(double)0.0;
 int inum=0, ic=0, ival=0;
 long lstat=0;
 char obsID[DHS_IMPL_MAXSTR];
 (void) memset(obsID,'\0',DHS_IMPL_MAXSTR);
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* check handle */
 if ( Tcl_GetInt(interp,argv[1],&ival) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsIOctlTcl-E-bad handle\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 eID = (dhsHandle)ival;
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::IOctl: eID=%d\n",(int)eID); (void) fflush(stderr);
 #endif
 /* check ioctl */
 if ( Tcl_GetInt(interp,argv[2],(int *)&inum) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsIOctlTcl-E-bad ioctl\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::IOctl: ionum=%d\n",inum); (void) fflush(stderr);
 #endif
 /* check expID */
 if ( Tcl_GetDouble(interp,argv[3],&expID) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsIOctlTcl-E-bad exposure id\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::IOctl: expID=%lf\n",expID); (void) fflush(stderr);
 #endif
 /* check obsID */
 for ( ic=4; ic<argc; ic++ ) { strcat(obsID,argv[ic]); strcat(obsID," "); }
 obsID[strlen(obsID)-1] = '\0';
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::IOctl: obsID=%s\n",obsID); (void) fflush(stderr);
 #endif
 /* execute the dhs function */
 dhsIOCtl(&lstat,response,eID,(long)inum,&expID,obsID,NULL);
 if ( STATUS_BAD(lstat) ) {
  (void) Tcl_SetResult(interp,response,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::IOctl: lstat=%ld\n",lstat); (void) fflush(stderr);
 #endif
 /* return result */  
 (void) sprintf(result,"%ld",lstat);
 (void) Tcl_SetResult(interp,result,TCL_STATIC);
 return TCL_OK;
}

/*******************************************************************************
 * dhsMetaDataTcl ( ... )
 *  Use: set dhsStat [dhs::MetaData <eID> <{data}> <nlines> <{cfgList}> <expID> <obsID>]
 *******************************************************************************/
static int dhsMetaDataTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 char *fp=(char *)NULL, *ap=(char *)NULL, *cp=(char *)NULL;
 char **lsArgvC=(char **)NULL, **lsArgvD=(char **)NULL, **lsArgvF=(char **)NULL, **lsArgvS=(char **)NULL;
 dhsHandle eID=(dhsHandle)0;
 double expID=(double)0.0;
 int ival=0, ik=0, ic=0, ierror=0, nbytes=0, nlines=0, lsArgcC=0, lsArgcD=0, lsArgcF=0, lsArgcS=0;
 long lstat=0;
 mdConfig_t mdConfigTcl;
 char fitsName[DHS_FITS_NAMESIZE], fitsValue[DHS_FITS_VALSIZE], fitsComment[DHS_FITS_COMMENT];
 char avpName[DHS_AVP_NAMESIZE], avpValue[DHS_AVP_VALSIZE], avpComment[DHS_AVP_COMMENT], obsID[DHS_IMPL_MAXSTR];
 (void) memset((void *)&mdConfigTcl,0,sizeof(mdConfig_t));
 (void) memset(fitsName,'\0',DHS_FITS_NAMESIZE);
 (void) memset(fitsValue,'\0',DHS_FITS_VALSIZE);
 (void) memset(fitsComment,'\0',DHS_FITS_COMMENT);
 (void) memset(avpName,'\0',DHS_AVP_NAMESIZE);
 (void) memset(avpValue,'\0',DHS_AVP_VALSIZE);
 (void) memset(avpComment,'\0',DHS_AVP_COMMENT);
 (void) memset(obsID,'\0',DHS_IMPL_MAXSTR);
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* check handle */
 if ( Tcl_GetInt(interp,argv[1],&ival) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsMetaDataTcl-E-bad handle\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 eID = (dhsHandle)ival;
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::MetaData: eID=%d\n",(int)eID); (void) fflush(stderr);
 #endif
 /* check data list */
 if ( Tcl_SplitList(interp,argv[2],&lsArgcD,(tclListP_t)&lsArgvD)!=TCL_OK ) {
  (void) sprintf(result,"%s","dhsMetaDataTcl-E-bad data list\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 /* check nlines */
 if ( Tcl_GetInt(interp,argv[3],&nlines) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsMetaDataTcl-E-bad nlines\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 } 
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::MetaData: nlines=%d\n",nlines); (void) fflush(stderr);
 #endif
 /* check configuration list */
 if ( Tcl_SplitList(interp,argv[4],&lsArgcC,(tclListP_t)&lsArgvC)!=TCL_OK || lsArgcC!=4L ) {
  (void) sprintf(result,"%s","dhsMetaDataTcl-E-bad configuration list\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 /* check expID */
 if ( Tcl_GetDouble(interp,argv[5],&expID) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsMetaDataTcl-E-bad exposure id\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::MetaData: expID=%lf\n",expID); (void) fflush(stderr);
 #endif
 /* check obsID */
 for ( ic=6; ic<argc; ic++ ) { strcat(obsID,argv[ic]); strcat(obsID," "); }
 obsID[strlen(obsID)-1] = '\0';
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::MetaData: obsID=%s\n",obsID); (void) fflush(stderr);
 #endif
 /* set configuration */
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[0],&ival) != TCL_OK ) ierror = DHS_TRUE; mdConfigTcl.metaType = (XLONG) ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[1],&ival) != TCL_OK ) ierror = DHS_TRUE; mdConfigTcl.numFields = (XLONG) ival;
 if ( Tcl_SplitList(interp,lsArgvC[2],&lsArgcF,(tclListP_t)&lsArgvF) != TCL_OK ) ierror = DHS_TRUE;
 if ( Tcl_SplitList(interp,lsArgvC[3],&lsArgcS,(tclListP_t)&lsArgvS) != TCL_OK ) ierror = DHS_TRUE;
 if ( ierror ) {
  (void) sprintf(result,"%s","dhsMetaDataTcl-E-bad list element\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::MetaData: mdConfigTcl: metaType=%d, numFields=%d\n",(int)mdConfigTcl.metaType,(int)mdConfigTcl.numFields); (void) fflush(stderr);
 #endif
 for ( ic=0; ic<mdConfigTcl.numFields; ic++ ) {
  ival=0; if ( Tcl_GetInt(interp,lsArgvF[ic],&ival) != TCL_OK ) ierror = DHS_TRUE; mdConfigTcl.fieldSize[ic] = (XLONG) ival;
  ival=0; if ( Tcl_GetInt(interp,lsArgvS[ic],&ival) != TCL_OK ) ierror = DHS_TRUE; mdConfigTcl.dataType[ic] = (XLONG) ival;
  if ( ierror ) {
   (void) sprintf(result,"%s","dhsMetaDataTcl-E-bad configuration element\n");
   (void) Tcl_SetResult(interp,result,TCL_STATIC);
   return TCL_ERROR;
   }
   #ifdef DEBUGTCL
    (void) fprintf(stderr,"dhs::MetaData: mdConfigTcl: fieldSize[%d]=%d, dataType[%d]=%d\n",ic,(int)mdConfigTcl.fieldSize[ic],ic,(int)mdConfigTcl.dataType[ic]); (void) fflush(stderr);
   #endif
 }
 /* get memory */
 switch (mdConfigTcl.metaType) {
  case DHS_MDTYPE_FITSHEADER:
   nbytes = nlines*DHS_FITS_RAWLEN;
   if ( (cp=fp=(char *)Tcl_Alloc(nbytes)) == (char *)NULL ) {
    (void) sprintf(result,"%s","dhsMetaDataTcl-E-bad alloc\n");
    (void) Tcl_SetResult(interp,result,TCL_STATIC);
    return TCL_ERROR;
   }
   (void) memset((void *)fp,' ',nbytes);
   for ( ic=0, ik=0; ik<nlines*3; ik+=3 ) {
    (void) sprintf((char *)fitsName,"%8s",lsArgvD[ik]);
    (void) memmove((void *)fp,fitsName,DHS_FITS_NAMESIZE);   fp += DHS_FITS_NAMESIZE;
    (void) sprintf((char *)fitsValue,"%20s",lsArgvD[ik+1]);
    (void) memmove((void *)fp,fitsValue,DHS_FITS_VALSIZE);   fp += DHS_FITS_VALSIZE;
    (void) sprintf((char *)fitsComment,"%46s",lsArgvD[ik+2]);
    (void) memmove((void *)fp,fitsComment,DHS_FITS_COMMENT); fp += DHS_FITS_COMMENT;
   }
   break;
  case DHS_MDTYPE_AVPAIR:
   nbytes = nlines*DHS_AVP_RAWLEN;
   if ( (cp=ap=(char *)Tcl_Alloc(nbytes)) == (char *)NULL ) {
    (void) sprintf(result,"%s","dhsMetaDataTcl-E-bad alloc\n");
    (void) Tcl_SetResult(interp,result,TCL_STATIC);
    return TCL_ERROR;
   }
   (void) memset((void *)ap,' ',nbytes);
   for ( ic=0, ik=0; ik<nlines*3; ik+=3 ) {
    (void) sprintf((char *)avpName,"%32s",lsArgvD[ik]);
    (void) memmove((void *)ap,avpName,DHS_AVP_NAMESIZE);   ap += DHS_AVP_NAMESIZE;
    (void) sprintf((char *)avpValue,"%32s",lsArgvD[ik+1]);
    (void) memmove((void *)ap,avpValue,DHS_AVP_VALSIZE);   ap += DHS_AVP_VALSIZE;
    (void) sprintf((char *)avpComment,"%64s",lsArgvD[ik+2]);
    (void) memmove((void *)ap,avpComment,DHS_AVP_COMMENT); ap += DHS_AVP_COMMENT;
   }
   break;
  default:
   (void) sprintf(result,"%s","dhsMetaDataTcl-E-bad data type\n");
   (void) Tcl_SetResult(interp,result,TCL_STATIC);
   return TCL_ERROR;
   break;
 }
 /* execute the dhs function */
 dhsSendMetaData(&lstat,response,eID,(void *)cp,(size_t)nbytes,&mdConfigTcl,&expID,obsID);
 if ( STATUS_BAD(lstat) ) {
  (void) Tcl_SetResult(interp,response,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::MetaData: lstat=%ld\n",lstat); (void) fflush(stderr);
 #endif
 /* return result */
 (void) sprintf(result,"%ld",lstat);
 (void) Tcl_SetResult(interp,result,TCL_STATIC);
 (void) Tcl_Free((char *)lsArgvC);
 (void) Tcl_Free((char *)lsArgvD);
 (void) Tcl_Free((char *)lsArgvF);
 (void) Tcl_Free((char *)lsArgvS);
 (void) Tcl_Free((char *)cp);
 return TCL_OK;
}

/*******************************************************************************
 * dhsPixelDataTcl ( ... )
 *  Use: set dhsStat [dhs::PixelData <eID> <{data}> <nelms> <{cfgList}> <expID> <obsID>]
 *******************************************************************************/
static int dhsPixelDataTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 char **lsArgvC=(char **)NULL, **lsArgvD=(char **)NULL;
 dhsHandle eID=(dhsHandle)0;
 double expID=(double)0.0;
 int ival=0, ik=0, ic=0, ierror=0, nbytes=0, nelms=0, lsArgcC=0, lsArgcD=0;
 long lstat=0;
 XLONG *ip=(XLONG *)NULL, *dp=(XLONG *)NULL;
 fpConfig_t fpConfigTcl;
 char obsID[DHS_IMPL_MAXSTR];
 (void) memset((void *)&fpConfigTcl,0,sizeof(fpConfig_t));
 (void) memset(obsID,'\0',DHS_IMPL_MAXSTR);
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* check handle */
 ival = 0;
 if ( Tcl_GetInt(interp,argv[1],&ival) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsPixelDataTcl-E-bad handle\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 eID = (dhsHandle)ival;
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::PixelData: eID=%d\n",(int)eID); (void) fflush(stderr);
 #endif
 /* check data list */
 if ( Tcl_SplitList(interp,argv[2],&lsArgcD,(tclListP_t)&lsArgvD)!=TCL_OK ) {
  (void) sprintf(result,"%s","dhsPixelDataTcl-E-bad data list\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 /* check nelms */
 if ( Tcl_GetInt(interp,argv[3],&nelms) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsPixelDataTcl-E-bad nelms\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::PixelData: nelms=%d\n",nelms); (void) fflush(stderr);
 #endif
 /* check configuration list */
 if ( Tcl_SplitList(interp,argv[4],&lsArgcC,(tclListP_t)&lsArgvC)!=TCL_OK || lsArgcC!=11L ) {
  (void) sprintf(result,"%s","dhsPixelDataTcl-E-bad configuration list\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 /* check expID */
 if ( Tcl_GetDouble(interp,argv[5],&expID) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsPixelDataTcl-E-bad exposure id\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::PixelData: expID=%lf\n",expID); (void) fflush(stderr);
 #endif
 /* check obsID */
 for ( ic=6; ic<argc; ic++ ) { strcat(obsID,argv[ic]); strcat(obsID," "); }
 obsID[strlen(obsID)-1] = '\0';
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::PixelData: obsID=%s\n",obsID); (void) fflush(stderr);
 #endif
 /* set configuration */
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[0],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xSize    = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[1],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.ySize    = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[2],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xStart   = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[3],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yStart   = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[4],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.dataType = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[5],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xDir     = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[6],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yDir     = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[7],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xDetSz   = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[8],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yDetSz   = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[9],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xDetCnt  = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[10],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yDetCnt  = (XLONG)ival;
 if ( ierror ) {
  (void) sprintf(result,"%s","dhsPixelDataTcl-E-bad list element\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::PixelData: fpConfigTcl.xSize    =%d\n",(int)fpConfigTcl.xSize);
  (void) fprintf(stderr,"dhs::PixelData: fpConfigTcl.ySize    =%d\n",(int)fpConfigTcl.ySize);
  (void) fprintf(stderr,"dhs::PixelData: fpConfigTcl.xStart   =%d\n",(int)fpConfigTcl.xStart);
  (void) fprintf(stderr,"dhs::PixelData: fpConfigTcl.yStart   =%d\n",(int)fpConfigTcl.yStart);
  (void) fprintf(stderr,"dhs::PixelData: fpConfigTcl.dataType =%d\n",(int)fpConfigTcl.dataType);
  (void) fprintf(stderr,"dhs::PixelData: fpConfigTcl.xDir     =%d\n",(int)fpConfigTcl.xDir);
  (void) fprintf(stderr,"dhs::PixelData: fpConfigTcl.yDir     =%d\n",(int)fpConfigTcl.yDir);
  (void) fprintf(stderr,"dhs::PixelData: fpConfigTcl.xDetSz   =%d\n",(int)fpConfigTcl.xDetSz);
  (void) fprintf(stderr,"dhs::PixelData: fpConfigTcl.yDetSz   =%d\n",(int)fpConfigTcl.yDetSz);
  (void) fprintf(stderr,"dhs::PixelData: fpConfigTcl.xDetCnt  =%d\n",(int)fpConfigTcl.xDetCnt);
  (void) fprintf(stderr,"dhs::PixelData: fpConfigTcl.yDetCnt  =%d\n",(int)fpConfigTcl.yDetCnt);
  (void) fflush(stderr);
 #endif
 /* get memory */
 nbytes = nelms * sizeof(XLONG);
 if ( (ip=dp=(XLONG *)Tcl_Alloc(nbytes)) == (XLONG *)NULL ) {
  (void) sprintf(result,"%s","dhsPixelDataTcl-E-bad alloc\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 /* extract data by re-using lsArgcC/lsArgvC */
 (void) memset((void *)dp,0,nbytes);
 for ( ic=0; ic<lsArgcD; ic++ ) {
  if ( Tcl_SplitList(interp,lsArgvD[ic],&lsArgcC,(tclListP_t)&lsArgvC) != TCL_OK ) {
   (void) sprintf(result,"%s","dhsPixelDataTcl-E-bad internal list\n");
   (void) Tcl_SetResult(interp,result,TCL_STATIC);
   return TCL_ERROR;
  }
  for ( ik=0; ik<lsArgcC; ik++ ) {
   ival = 0;
   if ( Tcl_GetInt(interp,lsArgvC[ik],&ival) != TCL_OK ) {
    (void) sprintf(result,"%s","dhsPixelDataTcl-E-bad array data\n");
    (void) Tcl_SetResult(interp,result,TCL_STATIC);
    return TCL_ERROR;
   }
   *dp = (XLONG)ival;
   dp++;
  }
 }
 /* execute the dhs function */
 dhsSendPixelData(&lstat,response,eID,(void *)ip,(size_t)nbytes,&fpConfigTcl,&expID,obsID);
 if ( STATUS_BAD(lstat) ) {
  (void) Tcl_SetResult(interp,response,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::PixelData: lstat=%ld\n",lstat); (void) fflush(stderr);
 #endif
 /* return result */
 (void) sprintf(result,"%ld",lstat);
 (void) Tcl_SetResult(interp,result,TCL_STATIC);
 (void) Tcl_Free((char *)lsArgvC);
 (void) Tcl_Free((char *)lsArgvD);
 (void) Tcl_Free((char *)ip);
 return TCL_OK;
}

/*******************************************************************************
 * dhsOpenExpTcl ( ... )
 *  Use: set eID [dhs::OpenExp <cID> <{cfgList}> <expID> <obsID>]
 *******************************************************************************/
static int dhsOpenExpTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 char **lsArgvC=(char **)NULL;
 dhsHandle cID=(dhsHandle)0;
 double expID=(double)0.0;
 int ival=0, ic=0, ierror=0, lsArgC=0;
 long lstat=0;
 fpConfig_t fpConfigTcl;
 char obsID[DHS_IMPL_MAXSTR];
 (void) memset((void *)&fpConfigTcl,0,sizeof(fpConfig_t));
 (void) memset(obsID,'\0',DHS_IMPL_MAXSTR);
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* check handle */
 if ( Tcl_GetInt(interp,argv[1],&ival) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsOpenExpTcl-E-bad handle\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 cID = (dhsHandle)ival;
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::OpenExp>> cID=%d\n",(int)cID); (void) fflush(stderr);
 #endif
 /* check configuration list */
 if ( Tcl_SplitList(interp,argv[2],&lsArgC,(tclListP_t)&lsArgvC)!=TCL_OK || lsArgC<11 ) {
  (void) sprintf(result,"%s","dhsOpenExpTcl-E-bad list\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 /* check expID */
 if ( Tcl_GetDouble(interp,argv[3],&expID) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsOpenExpTcl-E-bad exposure id\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::OpenExp>> expID=%lf\n",expID); (void) fflush(stderr);
 #endif
 /* check obsID */
 for ( ic=4; ic<argc; ic++ ) { strcat(obsID,argv[ic]); strcat(obsID," "); }
 obsID[strlen(obsID)-1] = '\0';
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::OpenExp>> obsID=%s\n",obsID); (void) fflush(stderr);
 #endif
 /* set configuration */
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[0],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xSize = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[1],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.ySize = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[2],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xStart = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[3],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yStart = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[4],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.dataType = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[5],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xDir = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[6],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yDir = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[7],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xDetSz = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[8],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yDetSz = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[9],&ival)  != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xDetCnt = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[10],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yDetCnt = (XLONG)ival;
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::OpenExp>> fpConfigTcl.xSize=%d\n",(int)fpConfigTcl.xSize); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenExp>> fpConfigTcl.ySize=%d\n",(int)fpConfigTcl.ySize); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenExp>> fpConfigTcl.xStart=%d\n",(int)fpConfigTcl.xStart); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenExp>> fpConfigTcl.yStart=%d\n",(int)fpConfigTcl.yStart); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenExp>> fpConfigTcl.dataType=%d\n",(int)fpConfigTcl.dataType); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenExp>> fpConfigTcl.xDir=%d\n",(int)fpConfigTcl.xDir); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenExp>> fpConfigTcl.yDir=%d\n",(int)fpConfigTcl.yDir); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenExp>> fpConfigTcl.xDetSz=%d\n",(int)fpConfigTcl.xDetSz); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenExp>> fpConfigTcl.yDetSz=%d\n",(int)fpConfigTcl.yDetSz); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenExp>> fpConfigTcl.xDetCnt=%d\n",(int)fpConfigTcl.xDetCnt); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenExp>> fpConfigTcl.yDetCnt=%d\n",(int)fpConfigTcl.yDetCnt); (void) fflush(stderr);
 #endif
 if ( ierror ) {
  (void) sprintf(result,"%s","dhsOpenExpTcl-E-bad list element\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 /* execute the dhs function */
 dhsOpenExp(&lstat,response,cID,&fpConfigTcl,&expID,obsID);
 if ( STATUS_BAD(lstat) ) {
  (void) Tcl_SetResult(interp,response,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::OpenExp: lstat=%ld\n",lstat); (void) fflush(stderr);
 #endif
 /* return result */
 (void) sprintf(result,"%ld",lstat);
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::OpenExp>> response=\"%s\"\n",response); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenExp>> result=\"%s\"\n",result); (void) fflush(stderr);
 #endif
 (void) Tcl_SetResult(interp,result,TCL_STATIC);
 (void) Tcl_Free((char *)lsArgvC);
 return TCL_OK;
}

/*******************************************************************************
 * dhsCloseExpTcl ( ... )
 *  Use: set dhsStat [dhs::CloseExp <eID> <expID>]
 *******************************************************************************/
static int dhsCloseExpTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 dhsHandle eID=(dhsHandle)0;
 double expID=(double)0.0;
 int ival=0;
 long lstat=0;
 char obsID[DHS_IMPL_MAXSTR];
 (void) memset(obsID,'\0',DHS_IMPL_MAXSTR);
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* check handle */
 if ( Tcl_GetInt(interp,argv[1],&ival) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsCloseExpTcl-E-bad handle\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 eID = (dhsHandle)ival;
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::CloseExp>> eID=%d\n",(int)eID); (void) fflush(stderr);
 #endif
 /* check expID */
 if ( Tcl_GetDouble(interp,argv[2],&expID) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsCloseExpTcl-E-bad exposure id\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::CloseExp>> expID=%lf\n",expID); (void) fflush(stderr);
 #endif
 /* execute the dhs function */
 dhsCloseExp(&lstat,response,eID,expID);
 if ( STATUS_BAD(lstat) ) {
  (void) Tcl_SetResult(interp,response,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::CloseExp>> lstat=%ld\n",lstat); (void) fflush(stderr);
 #endif
 /* return result */  
 (void) sprintf(result,"%ld",lstat);
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::CloseExp>> response=\"%s\"\n",response); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::CloseExp>> result=\"%s\"\n",result); (void) fflush(stderr);
 #endif
 (void) Tcl_SetResult(interp,result,TCL_STATIC);
 return TCL_OK;
}

/*******************************************************************************
 * dhsOpenConnect ( ... )
 *  Use: set cID [dhs::OpenConnect <systemID> <{cfgList}>]
 *******************************************************************************/
static int dhsOpenConnectTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 char **lsArgvC=(char **)NULL;
 dhsHandle cID=(dhsHandle)0;
 int whoami=0, ierror=0, ival=0, lsArgC=0;
 long lstat=0;
 fpConfig_t fpConfigTcl;
 char obsID[DHS_IMPL_MAXSTR];
 (void) memset((void *)&fpConfigTcl,0,sizeof(fpConfig_t));
 (void) memset(obsID,'\0',DHS_IMPL_MAXSTR);
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* check systemID */
 if ( Tcl_GetInt(interp,argv[1],&whoami) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsOpenConnectTcl-E-bad system id\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::OpenConnect>> whoami=%d (0x%x)\n",whoami,(unsigned int)whoami); (void) fflush(stderr);
 #endif
 /* check configuration list */
 if ( Tcl_SplitList(interp,argv[2],&lsArgC,(tclListP_t)&lsArgvC)!=TCL_OK || lsArgC<11 ) {
  (void) sprintf(result,"%s","dhsOpenConnectTcl-E-bad list\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 /* set configuration */
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[0],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xSize = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[1],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.ySize = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[2],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xStart = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[3],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yStart = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[4],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.dataType = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[5],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xDir = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[6],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yDir = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[7],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xDetSz = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[8],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yDetSz = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[9],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.xDetCnt = (XLONG)ival;
 ival=0; if ( Tcl_GetInt(interp,lsArgvC[10],&ival) != TCL_OK ) ierror = DHS_TRUE; fpConfigTcl.yDetCnt = (XLONG)ival;
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::OpenConnect>> fpConfigTcl.xSize=%d\n",(int)fpConfigTcl.xSize); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenConnect>> fpConfigTcl.ySize=%d\n",(int)fpConfigTcl.ySize); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenConnect>> fpConfigTcl.xStart=%d\n",(int)fpConfigTcl.xStart); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenConnect>> fpConfigTcl.yStart=%d\n",(int)fpConfigTcl.yStart); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenConnect>> fpConfigTcl.dataType=%d\n",(int)fpConfigTcl.dataType); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenConnect>> fpConfigTcl.xDir=%d\n",(int)fpConfigTcl.xDir); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenConnect>> fpConfigTcl.yDir=%d\n",(int)fpConfigTcl.yDir); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenConnect>> fpConfigTcl.xDetSz=%d\n",(int)fpConfigTcl.xDetSz); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenConnect>> fpConfigTcl.yDetSz=%d\n",(int)fpConfigTcl.yDetSz); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenConnect>> fpConfigTcl.xDetCnt=%d\n",(int)fpConfigTcl.xDetCnt); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenConnect>> fpConfigTcl.yDetCnt=%d\n",(int)fpConfigTcl.yDetCnt); (void) fflush(stderr);
 #endif
 if ( ierror ) {
  (void) sprintf(result,"%s","dhsOpenConnectTcl-E-bad list element\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 /* execute the dhs function */
 dhsOpenConnect(&lstat,response,&cID,(long)whoami,&fpConfigTcl);
 if ( STATUS_BAD(lstat) ) {
  (void) Tcl_SetResult(interp,response,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::OpenConnect>> lstat=%ld\n",lstat); (void) fflush(stderr);
 #endif
 /* return result */
 (void) sprintf(result,"%d",(int)cID);
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::OpenConnect>> response=\"%s\"\n",response); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::OpenConnect>> result=\"%s\"\n",result); (void) fflush(stderr);
 #endif
 (void) Tcl_SetResult(interp,result,TCL_STATIC);
 (void) Tcl_Free((char *)lsArgvC);
 return TCL_OK;
}

/*******************************************************************************
 * dhsCloseConnectTcl ( ... )
 *  Use: set dhsStat [dhs::CloseConnect <cID>]
 *******************************************************************************/
static int dhsCloseConnectTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 dhsHandle cID=(dhsHandle)0;
 int ival=0;
 long lstat=0;
 char obsID[DHS_IMPL_MAXSTR];
 (void) memset(obsID,'\0',DHS_IMPL_MAXSTR);
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* check handle */
 if ( Tcl_GetInt(interp,argv[1],&ival) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsCloseConnectTcl-E-bad handle\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 cID = (dhsHandle)ival;
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::CloseConnect>> cID=%d\n",(int)cID); (void) fflush(stderr);
 #endif
 /* execute the dhs function */
 dhsCloseConnect(&lstat,response,cID);
 if ( STATUS_BAD(lstat) ) {
  (void) Tcl_SetResult(interp,response,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::CloseConnect>> lstat=%ld\n",lstat); (void) fflush(stderr);
 #endif
 /* return result */  
 (void) sprintf(result,"%ld",lstat);
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::CloseConnect>> response=\"%s\"\n",response); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::CloseConnect>> result=\"%s\"\n",result); (void) fflush(stderr);
 #endif
 (void) Tcl_SetResult(interp,result,TCL_STATIC);
 return TCL_OK;
}

/*******************************************************************************
 * dhsSysOpenTcl ( ... )
 *  Use: set sID [dhs::SysOpen <systemID>]
 *******************************************************************************/
static int dhsSysOpenTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 dhsHandle sID=(dhsHandle)0;
 int whoami=0;
 long lstat=0;
 char obsID[DHS_IMPL_MAXSTR];
 (void) memset(obsID,'\0',DHS_IMPL_MAXSTR);
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* check systemID */
 if ( Tcl_GetInt(interp,argv[1],&whoami) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsSysOpenTcl-E-bad system id\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::SysOpen>> whoami=%d (0x%x)\n",(XLONG)whoami,(unsigned XLONG)whoami); (void) fflush(stderr);
 #endif
 /* execute the dhs function */
 dhsSysOpen(&lstat,response,&sID,(long)whoami);
 if ( STATUS_BAD(lstat) ) {
  (void) Tcl_SetResult(interp,response,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::SysOpen>> lstat=%ld, sID=%d\n",lstat,(int)sID); (void) fflush(stderr);
 #endif
 /* return result */  
 (void) sprintf(result,"%d",(int)sID);
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::SysOpen>> response=\"%s\"\n",response); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::SysOpen>> result=\"%s\"\n",result); (void) fflush(stderr);
 #endif
 (void) Tcl_SetResult(interp,result,TCL_STATIC);
 return TCL_OK;
}

/*******************************************************************************
 * dhsSysCloseTcl ( ... )
 *  Use: set dhsStat [dhs::SysClose <sID>]
 *******************************************************************************/
static int dhsSysCloseTcl ( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] ) {
 /* declare local scope variable and initialize them */
 dhsHandle sID=(dhsHandle)0;
 int ival=0;
 long lstat=0;
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* check handle */
 if ( Tcl_GetInt(interp,argv[1],&ival) != TCL_OK ) {
  (void) sprintf(result,"%s","dhsSysCloseTcl-E-bad handle\n");
  (void) Tcl_SetResult(interp,result,TCL_STATIC);
  return TCL_ERROR;
 }
 sID = (dhsHandle)ival;
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::SysClose>> sID=%d\n",(int)sID); (void) fflush(stderr);
 #endif
 /* execute the dhs function */
 dhsSysClose(&lstat,response,sID);
 if ( STATUS_BAD(lstat) ) {
  (void) Tcl_SetResult(interp,response,TCL_STATIC);
  return TCL_ERROR;
 }
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::SysClose>> lstat=%ld\n",lstat); (void) fflush(stderr);
 #endif
 /* return result */  
 (void) sprintf(result,"%ld",lstat);
 #ifdef DEBUGTCL
  (void) fprintf(stderr,"dhs::SysClose>> response=\"%s\"\n",response); (void) fflush(stderr);
  (void) fprintf(stderr,"dhs::SysClose>> result=\"%s\"\n",result); (void) fflush(stderr);
 #endif
 (void) Tcl_SetResult(interp,result,TCL_STATIC);
 return TCL_OK;
}

/*******************************************************************************
 * dhstcl_Init ( ... )
 *******************************************************************************/
int Dhstcl_Init ( Tcl_Interp *interp ) {
 /* declare local scope variable and initialize them */
 int nc=0;
 /* initialize static variables */
 (void) memset(response,'\0',MAXMSG);
 (void) memset(result,'\0',DHS_RESULT_LEN);
 /* write banner */
 nc = sprintf(result,"\nDHS Package v%s by %s.\n%s\nThis release: %s\n",_dhsTcl_S_,_dhsTcl_A_,_dhsTcl_C_,_dhsTcl_D_);
 (void) Tcl_SetResult(interp,result,TCL_STATIC);
 /* make the wrapper(s) available to Tcl/tk */
 if ( Tcl_InitStubs(interp,TCL_STUB_VERSION,0) == (char *)NULL ) return TCL_ERROR;
 Tcl_CreateCommand(interp,"dhs::help",        (Tcl_CmdProc *)dhsHelpTcl,        (ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 Tcl_CreateCommand(interp,"dhs::version",     (Tcl_CmdProc *)dhsVersionTcl,     (ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 Tcl_CreateCommand(interp,"dhs::SysOpen",     (Tcl_CmdProc *)dhsSysOpenTcl,     (ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 Tcl_CreateCommand(interp,"dhs::SysClose",    (Tcl_CmdProc *)dhsSysCloseTcl,    (ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 Tcl_CreateCommand(interp,"dhs::OpenConnect", (Tcl_CmdProc *)dhsOpenConnectTcl, (ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 Tcl_CreateCommand(interp,"dhs::CloseConnect",(Tcl_CmdProc *)dhsCloseConnectTcl,(ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 Tcl_CreateCommand(interp,"dhs::OpenExp",     (Tcl_CmdProc *)dhsOpenExpTcl,     (ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 Tcl_CreateCommand(interp,"dhs::CloseExp",    (Tcl_CmdProc *)dhsCloseExpTcl,    (ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 Tcl_CreateCommand(interp,"dhs::IOctl",       (Tcl_CmdProc *)dhsIOctlTcl,       (ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 Tcl_CreateCommand(interp,"dhs::ReadImage",   (Tcl_CmdProc *)dhsReadImageTcl,   (ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 Tcl_CreateCommand(interp,"dhs::PixelData",   (Tcl_CmdProc *)dhsPixelDataTcl,   (ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 Tcl_CreateCommand(interp,"dhs::MetaData",    (Tcl_CmdProc *)dhsMetaDataTcl,    (ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
 /* provide the package */
 return Tcl_PkgProvide(interp,"dhs",_dhsTcl_S_);
}
