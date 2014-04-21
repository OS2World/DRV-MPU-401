/* HEADER.C - Device header

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
   09-Jan-97  Timur Tabi   Added DA_NEEDOPEN to ATTRIB
*/

#pragma data_seg ("_header","data");

#define INCL_NOPMAPI
#include <os2.h>

#include <include.h>
#include <devhelp.h>
#include "header.h"

void __far MPU401_StrategyHandler(void);
#ifdef DEBUG
void __far MPUTRACE_StrategyHandler(void);
#endif
ULONG __far __loadds __cdecl DDCMD_EntryPoint(void);

#define DA_CHAR         0x8000   // Character PDD
#define DA_IDCSET       0x4000   // IDC entry point set
#define DA_BLOCK        0x2000   // Block device driver
#define DA_SHARED       0x1000   // Shared device
#define DA_NEEDOPEN     0x800    // Open/Close required

#define DA_OS2DRVR      0x0080   // Standard OS/2 driver
#define DA_IOCTL2       0x0100   // Supports IOCTL2
#define DA_USESCAP      0x0180   // Uses Capabilities bits

#define DA_CLOCK        8        // Clock device
#define DA_NULL         4        // NULL device
#define DA_STDOUT       2        // STDOUT device
#define DA_STDIN        1        // STDIN device

#define DC_INITCPLT     0x10     // Supports Init Complete
#define DC_ADD          8        // ADD driver
#define DC_PARALLEL     4        // Supports parallel ports
#define DC_32BIT        2        // Supports 32-bit addressing
#define DC_IOCTL2       1        // Supports DosDevIOCtl2 and Shutdown (1C)

DEV_HEADER header[] =
{
   {
#ifdef DEBUG
      sizeof(DEV_HEADER),
#else
      -1,
#endif
      DA_CHAR | DA_IDCSET | DA_NEEDOPEN | DA_USESCAP,
      (PFNENTRY) MPU401_StrategyHandler,
      (PFNENTRY) DDCMD_EntryPoint,
      {'M','P','U','4','0','1','$',' '},
      0,0,
      DC_INITCPLT | DC_IOCTL2 | DC_32BIT
   }
#ifdef DEBUG
   ,{
      -1,
      DA_CHAR | DA_IDCSET | DA_NEEDOPEN | DA_USESCAP,
      (PFNENTRY) MPUTRACE_StrategyHandler,
      (PFNENTRY) 0,
      {'N','P','U','4','0','1','$',' '},
      0,0,
      DC_IOCTL2 | DC_32BIT
   }
#endif
};

#pragma code_seg ("_inittext");
#pragma data_seg ("_initdata","endds");

char szDevName[9] = {0};

int RegisterPDD(PFN pfnEntryPoint)
{
   unsigned i;

   for (i=0; i<8; i++) {
      if (header[0].abName[i] == ' ') break;
      szDevName[i] = header[0].abName[i];
   }

   if (i == 0)
      return FALSE;

   szDevName[i] = 0;
   if (DevHelp_RegisterPDD((PSZ) szDevName, (PFN) pfnEntryPoint))
      return FALSE;

   return TRUE;
}

