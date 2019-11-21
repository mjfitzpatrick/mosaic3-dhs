#!/bin/env /ndhs/bin/cl

string	expID = ""		# Exposure ID
string	raw = ""		# Raw image name
string	procDir = "."		# Processed user directory

string	trigdir = "nfpipe-01!MarioData/NEWFIRM/NEWFIRM_MTD/input/"
string	trigfile
string	retfile

# Scan arguments.
i = fscan (args, expID, raw, procDir)

raw = "nfdca!" // raw
path (raw) | scan (raw)
procDir = "newfirm!" // procDir
path (procDir) | scan (procDir)

# Log execution.
printf ("PLTrigger executed on ")
time
printf ("    expID=%s raw=%s pdir=%s\n", expID, raw, procDir)

s1 = substr (raw, 1, strstr(".fits",raw)-1)
if (access(s1//".trig")==NO)
   logout 0
;

# Send the filename and trigger files to the pipeline.
list = s1//".trig"
while (fscan (list, trigfile) != EOF) {
    i = strldx (".", trigfile)
    if (i == 0)
        next
    ;

    retfile = substr (trigfile, 1, i-1) // ".return"
    trigfile = trigdir // trigfile
    if (strstr("mtdend", trigfile) > 0) {
	printf ("    End file = %s\n", trigfile)
	sleep (1)
        touch (trigfile)
    } else {
	retfile = trigdir // "return/" // retfile
	printf ("    Trigger file = %s, return file = %s\n", trigfile, retfile)

	print (raw, > trigfile)
	print (procDir, > retfile)
	touch (trigfile//"trig")
	;
    }
}
list = ""; delete (s1//".trig")

logout 1
