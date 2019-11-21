procDCAData (smcPage_t *p)
{
    int   *idata = NULL;
    int   *ip = NULL, swapped = 0, nstreams = 1, s = 0, nbytes = 0;
    int    i=0, j=0, k=0, data_size=0, nx=0, ny=0, nchunks=128, datalen=0;
    int	   ccd_w = CCD_W, ccd_h = CCD_H, nbin = 0;
    int    row_count =0;
    smcSegment_t *seg = (smcSegment_t *) NULL; 
    mdConfig_t *mdCfg = smcGetMDConfig (p);
    fpConfig_t *fpCfg = smcGetFPConfig (p);
    double expID;
    char obsetID[80];
    DCA_Stream st;
#ifdef DO_TIMINGS
    time_t  t1, t2;
#endif


#ifdef FAKE_READOUT
	return;
#endif


    seg = SEG_ADDR(p);
    data_size = seg->dsize;

    /* See whether we're supposed to process the page or not.
    */
    if ( (procPages && strstr (procPages, seg->colID) == NULL) ||
	 (p->type == TY_META) )
	     return;

#ifdef DO_TIMINGS
    t1 = time ((time_t) NULL);
#endif
    expID = smcGetExpID (p);
    strcpy (obsetID, smcGetObsetID (p));
    idata = ip = (int *) smcGetPageData (p);

    ccd_w = fpCfg->xSize;		      /* use the fpConfig */
    ccd_h = fpCfg->ySize;
    nbin  = ccd_ysize / ccd_h;  /* ccd_ysize is CCD size without binning ?*/

    ip 		= idata;                      /* initialize   */
    ny 		= ccd_h;	              /* rows per CCD amp */
    nx 		= ccd_w;         	      /* cols per CCD amp */
    datalen 	= nx * CCD_BPP * CCD_ROWS;    /* CCD_ROWS is 32. Not sure why */
    swapped 	= 1;
    nstreams 	= 1;


    /* create and fill buffers with data from lower-left, lower-right, upper-left,
       and upper-right amps */



    if (console) {
        fprintf (stderr, "Proc DATA : 0x%x size=%d\n", 
		(int) seg, data_size);
        fprintf (stderr, "          : ExpID=%.6lf ObsID=%s seqno=%d\n",
	    expID, obsetID, seqno);
        fprintf (stderr, "          : mdCfg: type=%d  nfields=%d\n",
	    mdCfg->metaType, mdCfg->numFields);
        fprintf (stderr, "          : fpCfg: Start: %d,%d  Size: %d,%d\n", 
            fpCfg->xStart, fpCfg->yStart, fpCfg->xSize, fpCfg->ySize);
        fprintf (stderr, "          : page: colID='%s'  expPageNum=%d\n",
	    seg->colID, seg->expPageNum);
        fprintf (stderr, "          : seqno: %d dca_tid; %d\n", 
	    seg->seqno, dca_tid);
	fprintf (stderr, "          : ccd_w: %d ccd_h: %d [%d,%d] nbin: %d\n",
	    ccd_w, ccd_h, fpCfg->xSize, fpCfg->ySize, nbin);
	fprintf (stderr, "          : nx:    %d ny:    %d\n",nx,ny);
    }

    /* datalen     = datalen / (nbin); */
    nchunks 	= nchunks / nbin;

    st.npix 	= datalen / CCD_BPP;
    st.offset 	= 0;
    st.stride 	= 1;
    st.xydir 	= 0;
    st.x0 	= 0;
    st.y0 	= 0;


    /*  Set up the detector rasters to go into the image extensions using
    **  the usual convention of numbering from the LLC.  The output image
    **  will contain:
    */

    if(verbose)fprintf(stderr,"procDCAData: finding rasInfo for this segment page: %d\n",
	seg->expPageNum);

    k=-1;
    for (i=0; i < NCCDS; i++) {
#if 0
	if (strncmp (seg->colID, rasInfo[i].colID, 4) == 0 &&  /* assume colID = "PanA" */
	    seg->expPageNum == rasInfo[i].pageNum) { /* pageNum is same as buffer Index + 1 ? */
		st.destimage = rasInfo[i].destNum;
		k = i;
		break;
	}
#endif
        /* if destimage maps to display as follows:   
           13 14 15 16
	   9  10 11 12
           5   6  7  8
           1   2  3  4 

         then CCD 1 (ll, lr, ul, ur) maps to destimage 3, 4, 7, 8
              CCD 2 (ll, lr, ul, ur) maps to destimage 1, 2, 5, 6
              CCD 3 (ll, lr, ul, ur) maps to destimage 11,12,15,16
              CCD 4 (ll, lr, ul, ur) maps to destimage 9 ,10,13,14

	*/

//        if(verbose)fprintf(stderr,
//		"procDCAData: CCD index %d: rasInfo ColID = %s pageNum = %d destNum = %d\n",
//		i,rasInfo[i].colID,rasInfo[i].pageNum, rasInfo[i].destNum);

	if (strncmp (seg->colID, rasInfo[i].colID, 4) == 0 &&  /* assume colID = "PanA" */
	    seg->expPageNum == rasInfo[i].pageNum) { /* pageNum is same as buffer Index + 1 ? */
		st.destimage = rasInfo[i].destNum;
		k = i;
		break;
	}

    }

    if(k<0){
	fprintf(stderr,"procDCAData : ERROR : could not rasInfo with matching page number\n");
	return;
    }

    if(verbose)fprintf(stderr,
		"procDCAData: CCD index %d: rasInfo ColID = %s pageNum = %d destNum = %d\n",
		k,rasInfo[k].colID,rasInfo[k].pageNum, rasInfo[k].destNum);

    /*  Send the data. */


    st.destimage =rasInfo[k].destNum;

    ip=idata;
    if(verbose)fprintf(stderr,
		"procDCAData: CCD %d, sending %d rows of length %d cols,  %d at a time to destimage %d\n",
		k,ny,nx,CCD_ROWS,st.destimage);

    j=0; /* chunk count */
    for (row_count =0; row_count < ny; row_count = row_count +  CCD_ROWS){

        /* Initialize the WriteHeaderData request for the keyword group.
        */
        seqno = seg->seqno;
        pvm_initsend (PvmDataDefault);
        pvm_pkint (&seqno, 1, 1);
        pvm_pkint (&datalen, 1, 1);
        pvm_pkint (&swapped, 1, 1);
        pvm_pkint (&nstreams, 1, 1);

        st.y0 = (j * CCD_ROWS);	 /* destination row */
	st.xstep = st.ystep = 1;

	pvm_pkint ((int *)&st, sizeof(st)/sizeof(int), 1);
	pvm_pkbyte ((char *)ip, datalen, 1);
	pvm_send (dca_tid, DCA_WritePixelData);

#if 0
	if (console && verbose)
	    fprintf (stderr, "%d: (x,y) = (%d,%d) len=%d\n", j,
		st.x0, st.y0, datalen);
#endif
	if ((j % 16) == 0) { /* every 16 * CCD_ROWS rows */
	    pvm_initsend (PvmDataDefault);
	    pvm_pkint (&seqno, 1, 1);
	    pvm_send (dca_tid, DCA_Synchronize);

	    mb_timeout.tv_sec = MB_TIMEOUT;
	    mb_timeout.tv_usec = 0;
            if ((s = pvm_trecv (dca_tid, DCA_Synchronize, &mb_timeout)) <= 0) {
		if (console)
                    fprintf (stderr, "pxf: %s", (s == 0) ? 
                        "timeout waiting for response from DCA\n" :
                        "failed to synchronize with DCA");
            }
	}
	j++;
	ip += st.npix;
	nbytes += datalen;
    }

    if(row_count != ny )
	fprintf(stderr,"procDCAData: WARNING : only %d of %d rows sent for CCD %d\n",
		row_count,ny,k);


   

    /* Send final Sync.
    */
    pvm_initsend (PvmDataDefault);
    pvm_pkint (&seqno, 1, 1);
    pvm_send (dca_tid, DCA_Synchronize);
    mb_timeout.tv_sec = MB_TIMEOUT;
    mb_timeout.tv_usec = 0;
    if ((s=pvm_trecv (dca_tid, DCA_Synchronize, &mb_timeout)) <= 0) {
	if (console) {
            fprintf (stderr, "pxf: %s", (s == 0) ? 
                 "timeout waiting for response from DCA\n" :
                 "failed to synchronize with DCA");
        }
    }

    if (console) {
	fprintf (stderr, ": nchunks=%d  %d bytes  %d pixels\n",
		nchunks, nbytes, nbytes / 4);
    }

#ifdef DO_TIMINGS
    t2 = time ((time_t)NULL);
    fprintf (stderr, "Time [%d] procDCAData()\n", (t2-t1));
#endif

    if(lldata)free(lldata);
    if(lrdata)free(lrdata);
    if(uldata)free(uldata);
    if(urdata)free(urdata);

//  if (ldata)
//      free (ldata);		/* free amplifier pointers 	*/
//  if (rdata)
//      free (rdata);
}
