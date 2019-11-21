
/* $Id: compat.pvmtev.h,v 4.50 1998/04/09 21:12:18 kohl Exp $ */

/*
 *         XPVM version 1.1:  A Console and Monitor for PVM
 *           Oak Ridge National Laboratory, Oak Ridge TN.
 *           Authors:  James Arthur Kohl and G. A. Geist
 *                   (C) 1994 All Rights Reserved
 *
 *                              NOTICE
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted
 * provided that the above copyright notice appear in all copies and
 * that both the copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Neither the Institution, Oak Ridge National Laboratory, nor the
 * Authors make any representations about the suitability of this
 * software for any purpose.  This software is provided ``as is''
 * without express or implied warranty.
 *
 * XPVM was funded by the U.S. Department of Energy.
 */

/*
 *	compat.pvmtev.h
 *
 *	Old PVM 3.3 Libpvm tracing includes.
 *
 */

/* event mask kept as ascii string */

#define	TEV33_MASK_LENGTH	(TRC_OLD_TEV_MAX / 8 + 2)

typedef char Pvmtmask33[TEV33_MASK_LENGTH];

#define	TEV33_MASK_SET(m, k) (m[(k) / 8] |= (1 << (((k) / 2) & 3)))

#define	TEV33_MASK_UNSET(m, k) (m[(k) / 8] &= ~(1 << (((k) / 2) & 3)))

#define	TEV33_MASK_CHECK(m, k) (m[(k) / 8] & (1 << (((k) / 2) & 3)))

#define	TEV33_MASK_INIT(m) { \
	int tev_init_mask_i = TEV33_MASK_LENGTH-1; \
	m[tev_init_mask_i] = 0; \
	while (tev_init_mask_i-- > 0) \
		m[tev_init_mask_i] = '@'; \
	}

