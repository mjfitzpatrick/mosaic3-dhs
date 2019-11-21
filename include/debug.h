
#if !defined(_STDIO_H)
#include <stdio.h>
#endif

#if !defined(_DEBUG_H)
#define _DEBUG_H

#ifdef DEBUG
#define DPRINT(Level, Arg, Msg) if(Arg >= Level) {fprintf(stderr, "\t*DBG* %s", Msg); fflush(stderr);}
#else
#define DPRINT(Level, Arg, Msg)
#endif

#ifdef DEBUG
#define DPRINTF(Level, Arg, Msg, Val) if(Arg >= Level)\
				      {   char __dbgMsg[256],__dbgMsg2[256];\
					  sprintf(__dbgMsg, "\t*DBG* %s", Msg);\
					  sprintf(__dbgMsg2, __dbgMsg, Val);\
					  fputs (__dbgMsg2, stderr); \
					  fflush(stderr);\
				      }
#else
#define DPRINTF(Level, Arg, Msg, Val)
#endif

#endif
