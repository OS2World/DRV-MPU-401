/* PARAM.HPP - Parses command-line parameters
*/

#ifndef PARAM_INCLUDED
#define PARAM_INCLUDED

#include <include.h>
#include "constants.h"

// The MPU-401 base I/O ports specified on the command line.
extern USHORT ausCL_BaseIO[MAX_MPU401];

// The IRQ levels specified on the command line
extern USHORT ausCL_IRQ[MAX_MPU401];

// The device header name specified on the command line
extern char szCL_DevName[8];

// TRUE if /P and /I parameters were specified on the command line
extern unsigned int fParamsSpecified;

// if True, issue an INT3 at the beginning of the INIT section
extern unsigned int fInt3;

// True if Resource Manager I/O checks should be ignored (/O:NORMIO)
extern unsigned int fNoRMIO;

// True if Resource Manager IRQ checks should be ignored (/O:NORMIRQ)
extern unsigned int fNoRMIRQ;

// True if the I/O ranges shouldn't be checked (/O:NOCHECKIO)
// Note that if fNoCheckIO is TRUE, then we can't autodetect an IRQ
// because we need to do port I/O to find the IRQ.
extern unsigned int fNoCheckIO;

// True if the IRQ's shouldn't be autodetected (/O:NOCHECKIRQ)
extern unsigned int fNoCheckIRQ;

// True if we should not use TIMER0 even if we find it
extern unsigned int fNoTimer0;

// The value of the /R parameter
extern int iCL_Resolution;

// TRUE if we should only use the 1st MPU-401 that we find
extern unsigned int fOnlyOne;

// The number of address bits reported to RM
// Changed to 16 if /O:16BITS is specified or if MicroChannel
// The default used to be 10, but most modern devices do 16
extern USHORT usRMWordSize;

// The flags used for the I/O resource allocation
// Changed to RS_IO_SHARED if /O:SHAREDIO is specified
// Changed to RS_IO_MULTIPLEXED if /O:MULTIIO is specified
extern USHORT usRMIOFlag;

// The flags used for the IRQ resource allocation
// Changed to RS_IRQ_SHARED if /O:SHAREDIRQ is specified
// Changed to RS_IRQ_MULTIPLEXED if /O:MULTIIRQ is specified
extern USHORT usRMIRQFlag;

int GetParms(char __far *pszCmdLine);

#endif
