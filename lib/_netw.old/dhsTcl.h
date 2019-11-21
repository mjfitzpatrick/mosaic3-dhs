#if !defined(_dhsTcl_H_)
 #define _dhsTcl_H_ 1.1.0
 #define _dhsTcl_D_ "11 January, 2008."
 #define _dhsTcl_S_ "1.1.0"
 #define _dhsTcl_A_ "P. N. Daly"
 #define _dhsTcl_C_ "(C) AURA Inc, 2004. All rights reserved."
 
 #if !defined(_dhsUtil_H_)
   #include <dhsUtil.h>
 #endif
 
 #if !defined(_TCL)
  #include <tcl.h>
 #endif

 #ifdef TCL83
  typedef char ***tclListP_t;
 #endif
 #ifdef TCL84
  typedef CONST char ***tclListP_t;
 #endif

#endif
