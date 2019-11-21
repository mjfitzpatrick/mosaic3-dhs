/******************************************************************************************
 * __doc__ \subsection {dhsErrNo.h}
 * __doc__ \begin{description}
 * __doc__  \item[\sc use:] \emph{\#include ``dhsErrNo.h''}
 * __doc__  \item[\sc description:]  This file contains the DHS error code defines. These 
 * __doc__    are used dhsUtil Library "libdhsUtil.so" any user code which wishes to 
 * __doc__    interpret dhs error messages should also include this file.
 * __doc__  \item[\sc argument(s):]  not applicable
 * __doc__  \item[\sc return(s):] not applicable
 * __doc__  \item[\sc last modified:] Wednesday, 20080630
 * __doc__ \end{description} 
 * 
 * History:
 *	20080630 - created file for DHS library - ncb
 *
 ******************************************************************************/

#if !defined(_dhsErrNo_H_)
 #define _dhsErrNo_H_ 1.0.0
 #define _dhsErrNo_S_ "1.0.0"
 #if !defined(DHS_LIBRARY_ERR)
  #define DHS_LIBRARY_ERR   -1400
  #define DHS_SIMULATION_OK     8
 #endif
 #define DHS_SYSTEM_ERROR		DHS_LIBRARY_ERR-1
 #define DHS_BAD_PARAMETER		DHS_LIBRARY_ERR-2
 #define DHS_INVALID_HDRSZ		DHS_LIBRARY_ERR-3
 #define DHS_NO_DATA			DHS_LIBRARY_ERR-4
 #define DHS_BUSY			DHS_LIBRARY_ERR-5
 #define DHS_TIMEOUT			DHS_LIBRARY_ERR-7
 #define DHS_CALL_UNSUPPORTED		DHS_LIBRARY_ERR-8
 #define DHS_INSUFFICIENT_RESOURCES	DHS_LIBRARY_ERR-9
 #define DHS_CRC_ERROR			DHS_LIBRARY_ERR-10
 #define DHS_READOUT_FAILED		DHS_LIBRARY_ERR-11
 #define DHS_NOT_OPEN			DHS_LIBRARY_ERR-12
 #define DHS_ASYNC_MESSAGE		DHS_LIBRARY_ERR-13
 #define DHS_WRITE_ERROR		DHS_LIBRARY_ERR-14
 #define DHS_READ_ERROR			DHS_LIBRARY_ERR-15
 #define DHS_MSG_MISMATCH		DHS_LIBRARY_ERR-19
 #define DHS_NUMBYTES_ERR		DHS_LIBRARY_ERR-20
 #define DHS_ABORT			DHS_LIBRARY_ERR-21
#endif
