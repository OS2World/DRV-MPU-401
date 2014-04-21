/* VDD.CPP - VDD/PDD Communications

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   19-Sep-98  Timur Tabi   Creation
*/

#define INCL_NOPMAPI
#include <os2.h>
#include <include.h>
#include <devhelp.h>
#include <string.h>

#include "vdd.hpp"
#include "stream.hpp"
#include "mpu401.hpp"
#include "isr.hpp"
#include "trace.h"


ULONG   ulPDDHandle;     // PDD handle for VDD communication
ULONG   VDDIDC_Offset;   // 32 bit offset of VDD entry point ptr
USHORT  VDDIDC_Sel;      // 16 bit selector of 48bit VDD entry point ptr

unsigned int fInUse = FALSE;      // True if the VDD is using the device

unsigned int fEnableVDD = FALSE;  // True if VDD support is enabled

// Calls from VDD to PDD

#define VDD_PDD_MAX_FUNC   5

#define VDD_PDD_INIT          0        // Open request sent to PDD by OS
#define VDD_PDD_QUERY         1        // Insure PDD & VDD are of same level
#define VDD_PDD_HANDLE        2        // Inform PDD of its IDC handle
#define VDD_PDD_OPEN          3        // Request adapter owndership
#define VDD_PDD_CLOSE         4        // Relinquish HW back to PDD control
#define VDD_PDD_GET_INFO      5        // Provide address of port I/O info
#define VDD_PDD_TRUSTED_OPEN  6        // Trusted Open to PDD

// Calls from PDD to VDD

#define PDD_VDD_MAX_FUNC   1

#define PDD_VDD_INTERRUPT  0
#define PDD_VDD_CLOSE      1

/*************************************************************************/
/* VDM support for AudioPDD communication to VDD                         */
/*************************************************************************/

#pragma pack(1)

typedef struct _IORANGE
{
   ULONG ulPort;
   ULONG ulRange;
} IORANGE;

typedef struct _ADAPTERDATA
{
   USHORT  usIRQLevel;             // IRQ level of card
   USHORT  usDMALevel;             // DMA level of card
   ULONG   ulNumPorts;             // number of port ranges
   IORANGE range[MAX_MPU401];
} ADAPTERDATA;

ADAPTERDATA adapterdata;

extern "C" ULONG __cdecl PDDEntryPoint2(ULONG ulFunc, ULONG ul1, ULONG ul2)
{
   ASSERT(ulFunc <= VDD_PDD_MAX_FUNC);
   unsigned int i;
   ULONG rc = TRUE;
   MPU *pmpu;
   static unsigned int fInitialized = FALSE;

   switch (ulFunc) {
      case VDD_PDD_INIT:                  // Called when the VDD calls VDHOpenPDD
         VDDIDC_Sel = (USHORT) ul1;
         VDDIDC_Offset = ul2;
         _fmemset(&adapterdata, 0, sizeof(ADAPTERDATA));
         adapterdata.usIRQLevel = -1;
         adapterdata.usDMALevel = -1;

         pmpu = (MPU *) llMPU.Head();
         ASSERT(pmpu);

         rc = FALSE;
         if (ul1 && pmpu && fEnableVDD) {
            if (pmpu->irq()) {
               if (pmpu->irq()->isExclusive()) {
                  adapterdata.usIRQLevel = pmpu->irq()->irq();
                  rc = TRUE;
               }
            }
            i = 0;
            while (pmpu) {
               if (pmpu->isExclusive()) {
                  adapterdata.range[i].ulPort = pmpu->port();
                  adapterdata.range[i].ulRange = 2;
                  i++;
                  rc = TRUE;
               }
               pmpu = (MPU *) pmpu->plNext;
            }
            adapterdata.ulNumPorts = i;
         }

         Trace(VDD_PDD_INIT, ul1 | ((ULONG) i << 16));
         fInitialized = TRUE;
         break;
      case VDD_PDD_QUERY:
         rc = (VDD_PDD_MAX_FUNC << 8) | PDD_VDD_MAX_FUNC;
         Trace(TRACE_VDD_PDD_QUERY, rc);
         break;
      case VDD_PDD_HANDLE:
         Trace(TRACE_VDD_PDD_HANDLE, ul1);
         ulPDDHandle = ul1;
         break;
      case VDD_PDD_OPEN:
         if (fInUse || uNumStreams)
            rc = FALSE;
         else
            fInUse = TRUE;
         Trace(TRACE_VDD_PDD_OPEN, (((ULONG) uNumStreams) << 16) | fInUse);
         break;
      case VDD_PDD_CLOSE:
         Trace(TRACE_VDD_PDD_CLOSE, fInUse);
         fInUse = FALSE;
         break;
      case VDD_PDD_GET_INFO:
         rc = 0;
         LIN lin;
         USHORT usTemp = (USHORT) (void __near *) &adapterdata;
         if (!DevHelp_VirtToLin(_DS(), usTemp, &lin))
            rc = lin;
         ASSERT(rc);
         ASSERT(fInitialized);
         Trace(TRACE_VDD_PDD_INFO, rc);
         break;
   }

   return rc;
}

