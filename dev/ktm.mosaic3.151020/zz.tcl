# 		DCA Keyword Translation Module for Mosaic 1.1
# 
# Input:
#     inkwdbs:  A list of KWDB pointers.
# 	The expected databases have names NOCS_Pre, NOCS_Post, PanA_Pre,
# 	PanA_Post, PanB_Pre, PanB_Post.  Only the "post" database are used.
# 	The keywords from any additional unexpected databases will be copied
# 	to the PHU.
#     imageid:  A name to use in dprintf calls for the image being processed.
# 	This is typically the output image file name.
# 
# Parameters:
#     srcdir: Directory for source files.
#     fname: Filename for saving all keywords.  May be undefined to skip.
#     datafile: Filename of data file.
#     mapfiles: Array of mapping files.
# 	The expected mapping files are NOCS and Pan.
#     mapunknown: Include keywords not in mapping?
# 	If "yes" then the mapping is much slower so this should be used
# 	during development and testing, such as when there have been changes
# 	to the upstream metadata suppliers.  Once all possible metadata have
# 	been identified they should be included in the mapping and
# 	mapunknown should be set to "no" for operational use.
#     maplevel: Level of mapping to use.
# 	0 - Include only important metadata
# 	1 - Include possibly interesting metadata
# 	2 - Include all metadata
# 
# 	For development and testing use level 1 and 2 to identify metadata
# 	that should either be explicitly handled by the KTM or added to the
# 	end of the header.  For operational use level 0 should be used.
# 
# Output:
#     outkwdbs: A list of KWDB pointers with names:
# 	The expected names are phu, im[1-N]
# 	If a dependent input database is not found then no extension will
# 	be created.
# 
#     Diagnostic messages using the dprintf routine.
# 
# Mapping files:
#     Mapping files consists of lines with three or four whitespace delimited
#     words.  The first word is the name of keyword in the database being
#     mapped.  The second word is the mapped name of the keyword or '-' to use
#     the same name.  The purpose of this mapping is to map long keywords to
#     unique FITS keywords since there is no requirement that the metadata
#     suppliers use FITS-style keywords.  Note that further keyword mapping of
#     level 0 keywords may occur in the KTM.  The third column is a level
#     number to be used as described previously.  Lower numbers are more
#     important and likely to be included and higher numbers are more likely
#     to be ignored.  Only level 0 keywords are explicitly processed by the
#     KTM.  The fourth column is a comment string.  This needs to be quoted if
#     it contains blanks.  This comment, if present, will override the comment
#     in the input database.

set imageid	foo

dprintf 1 $imageid "begin keyword translation\n"


set tcl_precision 14
set srcdir /dhs/lib
#set srcdir /Users/valdes/iraf/nfktm/lib
set fname ${imageid}.dat
set datafile ${srcdir}/mosaic3.dat
set mapfiles(NOCS) ${srcdir}/NOCS.map
set mapfiles(Pan) ${srcdir}/Pan.map
set mapunknown yes
set maplevel 0

dprintf 1 $imageid "Sourcing TCL at ${srcdir}/mosaic3src.tcl ....\n"
source ${srcdir}/mosaic3src.tcl

dprintf 1 $imageid "Executing KTM ....\n"
if {[catch { mosaic3 } err]} {
    dprintf 1 $imageid "Error: $err\n"
}

dprintf 1 $imageid "keyword translation done\n"
