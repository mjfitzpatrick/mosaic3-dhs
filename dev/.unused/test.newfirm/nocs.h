struct {
char *keyw;
char *val;
char *comment;
} Header[] = {
{"NOHS","1.1.1","NOHS Version [No Units]"},
{"NOCUTC","20070101","NOCUTC ID [YYYYMMDD]"},
{"NOCDHS","DARK","DHS script name [No Units]"},
{"NOCDDOF","0","dither Dec offset [Arcseconds]"},
{"NOCDITER","0","dither iteration count [No Units]"},
{"NOCDPAT","5PX","dither pattern [No Units]"},
{"NOCDPOS","0","dither position [No Units]"},
{"NOCDREP","0","dither repetition count [No Units]"},
{"NOCDROF","0","dither RA offset [Arcseconds]"},
{"NOCFIL","Dark","filter (J|H|Ks|1056|1063|1644|2122|2164|Dark|Open) [No Units]"},
{"NOCID","0.0","observation ID a.k.a expID [MSD]"},
{"NOCGID","0.0","group identification number [MSD]"},
{"NOCMDOF","0","map Dec offset [Arcminutes]"},
{"NOCMITER","arcminutes","map iteration count [No Units]"},
{"NOCMPAT","5PX","map pattern [No Units]"},
{"NOCMPOS","0","map position [No Units]"},
{"NOCMREP","0","map repetition count [No Units]"},
{"NOCMROF","0","map RA offset [Arcminutes]"},
{"NOCNO","1","observation number in this sequence [No Units]"},
{"NOCNUM","'1'","observation number request [No Units]"},
{"NOCSKY","0","sky offset modulus [No Units]"},
{"NOCODEC","0","Dec offset [Arcseconds]"},
{"NOCORA","0","RA offset [Arcseconds]"},
{"NOCRSD","0.0","recipe date [MSD]"},
{"NOCTIM","1.0","requested integration time [Seconds]"},
{"NOCTOT","1","total number of observations in set [No Units]"},
{"NOCTYP","DARK","observation type [No Units]"},
{"NOCPROP","2007A-007","proposal ID [No Units]"},
{"NOCPI","Phil Daly","principal investigator [No Units]"},
{"NOCPIE","pnd@noao.edu","principal investigator emai address [No Units]"},
{"NOCOA","Phil Daly","observing assistant [No Units]"},
{"NOCOAE","pnd@noao.edu","observing assistant emai address [No Units]"},
{"NOCSYS","kpno_4m","system ID [No Units]"},
{"NOCOBJ","DARK","observed object [No Units]"},
{"NOCTEL","KPNO Mayall 4m","telescope [No Units]"},
{"NOCLAMP","off","dome flat lamp status (on|off)"},
{"NOCPOST","00:00:00.00 00:00:00.0 2007","ntcs_moveto ra dec epoch [No Units]"},
{"NOCOFFT","0 0","ntcs_offset ra dec offset [Arcsecond]"},
{"NOCFOCUS","0","ntcs_focus value [micron]"},
{"NOCNAME","0.0","file name for data set [No Units]"},
{"F8IDENT","1.0","control program id and version [No Units]"},
{"F8PID","100","process id of f8mid [No Units]"},
{"F8HOST","onion","host for f8mid [No Units]"},
{"F8LINK","down","serial link status (up|down) [No Units]"},
{"F8FOCUS","0","actual focus position from primary (+=incr, -=decr) [um]"},
{"F8FOCT","0","desired focus position [um]"},
{"F8TIP","0","actual N/S mirror tip angle (+=N, -=S) [Degrees]"},
{"F8TIPT","0","desired tip angle [Degrees]"},
{"F8TILT","0","actual E/W mirror tip angle (+=E, -=W) [Degrees]"},
{"F8TILTT","0","desired tilt angle [Degrees]"},
{"TCSLINK","down","TCS link (up|down) [No Units]"},
{"TCSHOST","onion","host for tcs4m [No Units]"},
{"TCSPID","101","process id of tcs4m [No Units]"},
{"TCSCOMPU","onion","name of tcs4m computer [No Units]"},
{"TCSUPDAT","off","tcs4m polling enable (on|off) [No Units]"},
{"UT","00:00:00","universal time [HH:MM:SS]"},
{"UTDATE","01/01/2007","universal date [MM/DD/YYYY]"},
{"ST","00:00:00","sidereal time [HH:MM:SS]"},
{"RA","00:00:00.00","right ascension [HH:MM:SS.SS]"},
{"DEC","00:00:00.0","declination [DD:MM:SS.S]"},
{"EPOCH","2007","epoch [YYYY]"},
{"RAPRE","00:00:00.00","right ascension preset [HH:MM:SS.SS]"},
{"DECPRE","00:00:00.0","declination preset [DD:MM:SS.S]"},
{"EPOCHPRE","2007","epoch preset [YYYY]"},
{"AZ","00:00:00","telescope azimuth [DD:MM:SS]"},
{"ALT","00:00:00","telescope altitude [HH:MM:SS]"},
{"EQUINOX","2007","equinox [YYYY]"},
{"ZD","0","zenith distance [Degrees]"},
{"AIRMASS","0","airmass [No Units]"},
{"PARALL","0","parallactic [No Units]"},
{"RAOFF","0","ra offset [Arcsecond]"},
{"DECOFF","0","dec offset [Arcsecond]"},
{"RAINST","0","ra instrument center [Arcsecond]"},
{"DECINST","0","dec instrument center [Arcsecond]"},
{"RAZERO","0","ra zero [Arcsecond]"},
{"DECZERO","0","dec zero [Arcsecond]"},
{"RAINDEX","0","ra index [Arcsecond]"},
{"DECINDEX","0","dec index [Arcsecond]"},
{"RADIFF","0","ra diff [Arcsecond]"},
{"DECDIFF","0","dec diff [Arcsecond]"},
{"TCPMODE","ready","telescope mode [No Units]"},
{"TCPTRACK","off","telescope tracking status [No Units]"},
{"TCPSLEW","off","telescope slew status [No Units]"},
{"FOCUS","0","focus [mm]"},
{"FOCI","0","telescope foci [No Units]"},
{"FIELD","0","telescope field [No Units]"},
{"TCPHA","0","telescope ha [No Units]"},
{"HASERVO","0","ha servo position [Degrees]"},
{"DECSERVO","0","dec servo position [Degrees]"},
{"ACTPRIM","off","primary mirror mode [No Units]"},
{"TCSPOS","00:00:00.00 00:00:00.0 2007","ra, dec and epoch of target position [No Units]"},
{"TCSOFST","0 0","ra, dec offset [Arcsecond]"},
{"ALRTSERV","off","servo alert [No Units]"},
{"ALRTHA","off","HA alert [No Units]"},
{"ALRTDEC","off","Dec alert [No Units]"},
{"ALRTSTOW","off","Stow alert [No Units]"},
{"DOMEAZ","0","dome position [Degrees]"},
{"DOMEERR","0","dome error as distance from target [Degrees]"},
{"DOMEMODE","off","dome mode [No Units]"},
{"DOMESTAT","ready","dome status [No Units]"},
{"TCPGDR","off","guider status (on|off) [No Units]"},
{"INSTR","newfirm","name of current instrument [No Units]"},
{"NFIDENT","ICS 1.0","control program id and version [No Units]"},
{"NFAUTH","Shelby Gott","author(s) of the nicc code [No Units]"},
{"NFDATE","01/01/2007","date of last modification [No Units]"},
{"NFSIM","1","simulation mode (1=on, 0=off) [No Units]"},
{"NFECPOS","close","detected position (open|close|between) [No Units]"},
{"NFECCMD","close","demand position (open|close) [No Units]"},
{"NFFW1POS","0","wheel 1 actual position (0|1|2|3|4|5|6|7|8) [No Units]"},
{"NFFW1","0","wheel 1 demand position (0|1|2|3|4|5|6|7|8) [No Units]"},
{"NFFW2POS","0","wheel 2 actual position (0|1|2|3|4|5|6|7|8) [No Units]"},
{"NFFW2","0","wheel 2 demand position (0|1|2|3|4|5|6|7|8) [No Units]"},
{"NFFILPOS","Dark","detected position name [No Units]"},
{"NFFILCMD","Dark","demand position name [No Units]"},
{"NFDETSET","0","setpoint for detector array temp control [K]"},
{"NFOSSSET","0","setpoint for oss temp control [K]"},
{"NFDETREG","off","detector array temp regulation (on|off) [No Units]"},
{"NFOSSREG","off","oss temp regulation (on|off) [No Units]"},
{"NFDETTMP","0","detector array temp measured [K]"},
{"NFOSSTMP","0","oss temp measured [K]"},
{"NFCHPWR1","off","cold head 1 power (on|off) [No Units]"},
{"NFCHPWR2","off","cold head 2 power (on|off) [No Units]"},
{"NFCHPWR3","off","cold head 3 power (on|off) [No Units]"},
{"NFTSRATE","0","temperature sampling rate [Seconds]"},
{"NFTEMP1","0","lens 2 rim [K]"},
{"NFTEMP2","0","lens 2 cell spring finger [K]"},
{"NFTEMP3","0","lens 3 rim [K]"},
{"NFTEMP4","0","lens 3 cell sprint finger [K]"},
{"NFTEMP5","0","upper housing exterior [K]"},
{"NFTEMP6","0","upper housing exterior [K]"},
{"NFTEMP7","0","upper housing exterior [K]"},
{"NFTEMP8","0","middle housing exterior [K]"},
{"NFTEMP9","0","middle housing exterior [K]"},
{"NFTEMP10","0","lens 4 rim [K]"},
{"NFTEMP11","0","lens 5 rim [K]"},
{"NFTEMP12","0","lens 6 rim [K]"},
{"NFTEMP13","0","lens 4-5-6 cell base [K]"},
{"NFTEMP14","0","middle housing interior [K]"},
{"NFTEMP15","0","fold mirror rear surface [K]"},
{"NFTEMP16","0","lens 7 rim [K]"},
{"NFTEMP17","0","lens 7 cell spring finger [K]"},
{"NFTEMP18","0","fold mirror housing exterior [K]"},
{"NFTEMP19","0","fold mirror housing exterior [K]"},
{"NFTEMP20","0","lens 8 rim [K]"},
{"NFTEMP21","0","lens 8 cell spring finger [K]"},
{"NFTEMP22","0","lower housing exterior [K]"},
{"NFTEMP23","0","lower housing exterior [K]"},
{"NFTEMP24","0","lower housing exterior [K]"},
{"NFTEMP25","0","lens 1 exterior surface [K]"},
{"NFTEMP26","0","lens 1 interior (vacuum) face [K]"},
{"NFTEMP27","0","environmental cover interior [K]"},
{"NFTEMP28","0","filter wheel motor, upper wheel [K]"},
{"NFTEMP29","0","filter wheel motor, lower wheel [K]"},
{"NFTEMP30","0","filter wheel housing interior [K]"},
{"NFTEMP31","0","collimated cold baffle tube upper [K]"},
{"NFTEMP32","0","collimated cold baffle tube lower [K]"},
{"NFTEMP33","0","tangent bar 1 close to dewar wall [K]"},
{"NFTEMP34","0","tangent bar 1 halfway along length [K]"},
{"NFTEMP15","0","tangent bar 1 close to oss hatch [K]"},
{"NFTEMP36","0","tangent bar 2 halfway along length [K]"},
{"NFTEMP37","0","tangent bar 3 halfway along length [K]"},
{"NFTEMP38","0","active radiation shield upper-end [K]"},
{"NFTEMP39","0","active radiation shield midpoint [K]"},
{"NFTEMP40","0","active radiation shield midpoint [K]"},
{"NFTEMP41","0","outer passive radiation shield [K]"},
{"NFTEMP42","0","cold head 1 65K stage [K]"},
{"NFTEMP43","0","cold head 1 10K stage [K]"},
{"NFTEMP44","0","cold head 2 65K stage [K]"},
{"NFTEMP45","0","cold head 2 10K stage [K]"},
{"NFTEMP46","0","cold head 3 65K stage [K]"},
{"NFTEMP47","0","cold head 3 10K stage [K]"},
{"NFTEMP48","0","array alignment mount outer plate [K]"},
{"NFTEMP49","0","array alignment invar baseplate [K]"},
{"NFTEMP50","0","array alignment thermal mass [K]"},
{"NFTEMP51","0","2K x 2K array A [K]"},
{"NFTEMP52","0","2K x 2K array B [K]"},
{"NFTEMP53","0","2K x 2K array C [K]"},
{"NFTEMP54","0","2K x 2K array D [K]"},
{"NFPS","0","pressure sensor [mBar]"},
{"NFVS","0","vacuum sensor [uTorr]"},
{"NFC1XPOS","stop","camera 1 X axis motion position (set|stop) [um]"},
{"NFC1XACC","0","camera 1 X axis motion acceleration [um/s/s]"},
{"NFC1XVEL","0","camera 1 X axis motion velocity [um/s]"},
{"NFC1YPOS","stop","camera 1 Y axis motion position (set|stop) [um]"},
{"NFC1YACC","0","camera 1 Y axis motion acceleration [um/s/s]"},
{"NFC1YVEL","0","camera 1 Y axis motion velocity [um/s]"},
{"NFC1FOCI","0","camera 1 focus motion (set|stop) [um]"},
{"NFC1LENS","stop","camera 1 corrector lens motion (set|stop) [No Units]"},
{"NFC2XPOS","stop","camera 2 X axis motion position (set|stop) [um]"},
{"NFC2XACC","0","camera 2 X axis motion acceleration [um/s/s]"},
{"NFC2XVEL","0","camera 2 X axis motion velocity [um/s]"},
{"NFC2YPOS","stop","camera 2 Y axis motion position (set|stop) [um]"},
{"NFC2YACC","0","camera 2 Y axis motion acceleration [um/s/s]"},
{"NFC2YVEL","0","camera 2 Y axis motion velocity [um/s]"},
{"NFC2FOCI","0","camera 2 focus motion (set|stop) [um]"},
{"NFC2LENS","stop","camera 2 corrector lens motion (set|stop) [No Units]"},
{"NFC1EXP","0","camera 1 exposure time [Seconds]"},
{"NFC1TEMP","0","camera 1 temperature setpoint [C]"},
{"NFC1OBS","dark","camera 1 observation type (dark|zero|object|flat) [No Units]"},
{"NFC1BIN","1","camera 1 on-chip binning (1|2|3) [No Units]"},
{"NFC1PWR","off","camera 1 power control (on|off) [No Units]"},
{"NFC1REGL","off","camera 1 thermoelectric cooling control (on|off) [No Units]"},
{"NFC1FILT","0","camera 1 filter [No Units]"},
{"NFC1AMB","0","camera 1 current ambient temperature [C]"},
{"NFC1GDR","off","camera 1 guider mode [No Units]"},
{"NFC2EXP","0","camera 2 exposure time [Seconds]"},
{"NFC2TEMP","0","camera 2 temperature setpoint [C]"},
{"NFC2OBS","dark","camera 2 observation type (dark|zero|object|flat) [No Units]"},
{"NFC2BIN","1","camera 2 on-chip binning (1|2|3) [No Units]"},
{"NFC2PWR","off","camera 2 power control (on|off) [No Units]"},
{"NFC2REGL","off","camera 2 thermoelectric cooling control (on|off) [No Units]"},
{"NFC2FILT","0","camera 2 filter [No Units]"},
{"NFC2AMB","0","camera 2 current ambient temperature [C]"},
{"NFC2GDR","off","camera 2 guider mode [No Units]"},
{"NFC1POS","00:00:00.00 00:00:00.0 2007","camera 1 target in RA Dec Epoch [HH:MM:SS.SS DD:MM:SS.S YYYY]"},
{"NFC2POS","00:00:00.00 00:00:00.0 2007","camera 2 target in RA Dec Epoch [HH:MM:SS.SS DD:MM:SS.S YYYY]"},
{NULL,NULL,NULL}
};
