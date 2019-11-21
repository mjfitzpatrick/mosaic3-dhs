/*******************************************************************************
 * 
 * __doc__ \section {The dhsUtilTcl <<VERSION>> Library}
 * __doc__ \subsection {dhsUtilTcl.h}
 * __doc__ \begin{description}
 * __doc__  \item[\sc use:] \emph{\#include ``dhsUtilTcl.h''}
 * __doc__  \item[\sc description:] this file contains all common code
 * __doc__    required by the functions can be called from Tcl/tk.
 * __doc__  \item[\sc last modified:] Thursday, 5 January 2004
 * __doc__  \item[\sc author(s):] Philip N. Daly (pnd@noao.edu)
 * __doc__  \item[\sc license:] (c) 2003--2004 AURA, Inc. All rights reserved. Released under the GPL.
 * __doc__ \end{description} 
 *
 ******************************************************************************/

#if !defined(_dhsTcl_H_)
 #define _dhsTcl_H_ 1.0.3
 #define _dhsTcl_D_ "3 October, 2007."
 #define _dhsTcl_S_ "1.0.3"
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
