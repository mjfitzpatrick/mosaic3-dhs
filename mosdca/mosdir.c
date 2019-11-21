#include <stdio.h>
#include <ctype.h>
#include <pvm3.h>
#include "dso.h"
#include "mosdca.h"


/*
 * MOSDIR -- Set the output directory used by MOSDCA.
 */

#ifdef SZ_FNAME
#undef SZ_FNAME
#endif
#define	SZ_FNAME	256

#ifdef SZ_PATHNAME
#undef SZ_PATHNAME
#endif
#define	SZ_PATHNAME	512

main (argc, argv)
int argc;
char **argv;
{
}

mosdir (newdir)
char *newdir;
{
	int mytid;

	/* Attach to the message bus.  */
	if ((mytid = pvm_mytid()) < 0) {
	    dprintf (0, "picfeed", "failed to connect to message bus\n");
	    status = ERR;
	    goto done;
	} else {
	    pvm_setopt (PvmRoute, PvmDontRoute);
	    env_seqno = mytid;
	}

	/* Open a connection to the remote DCA.  It is a fatal error if we
	 * can't connect to a DCA.  Reopening the connection each time
	 * allows the DCA to be restarted between readouts.
	 */
	if (env_direct) {
	    pvm_setopt (PvmRoute, PvmRouteDirect);
	    dprintf (2, "picfeed", "direct routing enabled\n");
	}

	/* Locate the DCA.  */
	info = pvm_lookup (MOSDCA, 0, &mosdca);
	if (info == PvmNoEntry) {
	    dprintf (0, "picfeed", "cannot locate the DCA server\n");
	    status = ERR;
	    goto done;
	} else
	    dprintf (2, "picfeed", "DCA server found at tid=%d\n", mosdca);

	/* Verify that we can talk to the server.  The GetMode request is
	 * made for our unique sequence number which should be inactive
	 * (this does not mean that other readouts are not active).  An
	 * active mode indicates that and old readout which we initiated
	 * readout with our sequence number) is hung and should be aborted.
	 */
	mode = GetMode (mosdca);
	if (mode > 0 && mode != M_INACTIVE) {
	    pvm_initsend (PvmDataDefault);
	    pvm_pkint (&seqno, 1, 1);
	    pvm_send (mosdca, DCA_AbortReadout);
	    GetStatus (mosdca, DCA_AbortReadout);
	}

	if ((mode = GetMode(mosdca)) == M_INACTIVE)
	    dprintf (1, "picfeed", "communication with DCA verified\n");
	else {
	    dprintf (0, "picfeed",
		"communication with DCA fails (mode=%d)\n", mode);
	    status = ERR;
	    goto done;
	}

	/* Reset the direct routing flag. */
	pvm_setopt (PvmRoute, PvmDontRoute);

	/* Send an advisory set directory request to the DCA (whether or not
	 * the DCA actually allows a client to determine the output directory
	 * is up to the DCA).
	 */
	pvm_initsend (PvmDataDefault);
	pvm_pkint (&seqno, 1, 1);
	pvm_pkstr ("directory");
	pvm_pkstr (env_cwd);
	pvm_send (mosdca, DCA_SetParam);
	/* Close connection to the message bus. */
	pvm_exit();

	return (0);
}
