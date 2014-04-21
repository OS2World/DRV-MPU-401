/* INIT.CPP - Initializes driver, parses command-line, finds MPU-401 hardware

   This module has three purposes:

   1. Initialize the driver.  Most other modules are initialized from here.
   2. Parse the command line.  A somewhat sophisticated command-line parser
      is implemented here.  The parser scans the command line and sets
      various variables (mostly booleans) as appropriate.
   3. Locates the MPU-401 hardware.  MPU-401 hardware consists of a base
      I/O address and an optional IRQ.  If an IRQ is not available, then it
      only means that MIDI data cannot be sent from the hardware to the
      driver.  The driver only supports interrupt-driven input.

   The user has the option of either specifying (via command-line options)
   the base I/O address and the IRQ's for up to 9 different MPU-401's, or
   allowing the driver to autodetect the hardware.

   It's important to keep in mind that a particular MPU-401 does not
   necessarily have an associated IRQ level.  MMPM does not support MIDI
   recording, so an IRQ is never used, and RTMIDI is capable of handling
   playback-only MIDI devices.  During RTMIDI Type A registration, if an
   IRQ for a particular MPU-401 does not exist, the driver registers for
   "input" only.  Note that "input" for RTMIDI is the same as "output" for
   MMPM, because RTMIDI looks at communication from the driver's point of
   view, whereas MMPM looks at communication from MMPM's point of view.
*/

#pragma code_seg ("_inittext");
#pragma data_seg ("_initdata","endds");

extern "C" {               // 16-bit header files are not C++ aware
#define INCL_DOSMISC
#define INCL_NOPMAPI
#include <os2.h>
#include "build_id.h"
}

#include <string.h>

#include <include.h>
#include <devhelp.h>
#include "trace.h"
#include "..\wpddlib\strategy.h"
#include "header.h"
#include "iodelay.h"
#include "mono.h"
#include "end.h"
#include "malloc.h"
#include "..\printf\printf.h"
#include "iprintf.h"
#include "rmhelp.hpp"
#include "vdd.hpp"

#include "isr.hpp"
#include "timer0.hpp"
#include "stream.hpp"
#include "event.hpp"
#include "midistrm.h"
#include "param.hpp"

// A list of ports that should be autodetected
// Most of these are for the MQX-32M
USHORT ausAutoDetectPort[] = {0x330, 0x300, 0x332, 0x334, 0x336, 0x200, 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0x270, 0x310, 0x320, 0x340, 0x350, 0x360, 0x370};
#define NUM_AUTODETECT  NUM_ELEMENTS(ausAutoDetectPort)

// If a spurious interrupt is detected, uiFlakyIRQ will be set to the IRQ
// level and usFlakyIO will be set to the I/O address we were checking when
// we got the interrupt.
unsigned uiFlakyIRQ = 0;
USHORT usFlakyIO = 0;

// Stuff for resource manager
ADAPTER *gpadpt;

DEVICESTRUCT devicestruct =
{
  (PSZ) "AUDIO_# MPU-401",             /* DevDescriptName   */
  DS_FIXED_LOGICALNAME,                /* DevFlags;         */
  DS_TYPE_AUDIO,                       /* DevType;          */
  NULL                                 /* pAdjunctList;     */
};

// Messages displayed during initialization:

#ifdef EXPIRE
char szExpired[] = "This beta has expired.\n";
#endif
char szSuccess[] = "Successfully installed.\n";
char szMCA[] = "MicroChannel machine.\n";
char szSMP[] = "SMP kernel found.\n";
char szPrintf[] = "PRINTF.SYS found.\n";
char szBadParam[] = "Bad Parameter.\n";
char szNoPort[] = "Port doesn't exist.\n";
char szIRQnoPort[] = "IRQ specified, base port not specified.\n";
char szAllocPort[] = "Error trying to reserve port %x and IRQ %i.\n";
char szNoMPU[] = "Error or no MPU-401's found.\n";
char szNoTimer0[] = "Couldn't find TIMER0.SYS, using system timer.\n";
char szFoundMPU[] = "MPU-401 #%i: Port=%x.\n";
char szFoundMPUIRQ[] = "MPU-401 #%i: Port=%x IRQ=%i.\n";
char szFoundMPUIRQ0[] = "MPU-401 #%i: Port=%x IRQ=TIMER0.\n";
char szResolution[] = "Resolution is %i millisecond(s).\n";
char szRMDriver[] = "RM: Can't allocate driver.\n";
char szRMAdapter[] = "RM: Can't allocate adapter.\n";
char szRMDevice[] = "RM: Can't allocate device.\n";
char szCheckingPorts[] = "Scanning I/O ports:\n";
char szMaxMPU[] = "Maximum of 9 MPU-401's already allocated.\n";
char szRMIRQ[] = "RM conflict on IRQ %i.\n";
char szNOIRQ[] = "Couldn't find specified IRQ.\n";
char szFlakyIRQ[] = "Warning: spurious interrupt on IRQ %i while checking I/O %x.\n";
char szNoIOandAuto[] = "Option NOCHECKIO invalid with base I/O address autodetection.\n";
char szNoIRQandAuto[] = "Option NOCHECKIRQ invalid with IRQ autodetection.\n";
char szNoVdd[] = "Unable to register VDD entry point.\n";

void CopyHeaderToString(char *pbHeader, char __far *psz)
{
   unsigned i;

   for (i=0; i<8; i++) {
      if (pbHeader[i] == ' ') {
         psz[i] = 0;
         return;
      }
      psz[i] = pbHeader[i];
   }

   psz[i] = 0;
}

int FindDriver(char __far *pszDevName)
{
#define OPEN_FLAG ( OPEN_ACTION_OPEN_IF_EXISTS )
#define OPEN_MODE ( OPEN_FLAGS_FAIL_ON_ERROR | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE )

   HFILE hfile;
   USHORT usAction = 0;

   if (DosOpen((PSZ) pszDevName, &hfile, &usAction, 0, 0, OPEN_FLAG, OPEN_MODE, NULL))
      return FALSE;

   DosClose(hfile);
   return TRUE;
}

void FindUniqueHeader(void)
{
   char szDevName[9];

   CopyHeaderToString(phdr->abName, szDevName);
   if (!FindDriver(szDevName))
      return;

   phdr->abName[7] = '$';
   for (BYTE b='A'; b <= 'Z'; b++) {
      phdr->abName[6] = b;
      CopyHeaderToString(phdr->abName, szDevName);
      if (!FindDriver(szDevName))
         return;
   }
}

USHORT usAutoStatPort;      // The I/O port to check
unsigned uAutoIRQ;          // The IRQ that was found

void AutodetectISR(unsigned);

void __far AutodetectISR3(void)
{
   AutodetectISR(3);
}

void __far AutodetectISR4(void)
{
   AutodetectISR(4);
}

void __far AutodetectISR5(void)
{
   AutodetectISR(5);
}

void __far AutodetectISR7(void)
{
   AutodetectISR(7);
}

void __far AutodetectISR9(void)
{
   AutodetectISR(9);
}

void __far AutodetectISR10(void)
{
   AutodetectISR(10);
}

void __far AutodetectISR11(void)
{
   AutodetectISR(11);
}

void __far AutodetectISR12(void)
{
   AutodetectISR(12);
}

void __far AutodetectISR14(void)
{
   AutodetectISR(14);
}

void __far AutodetectISR15(void)
{
   AutodetectISR(15);
}

typedef void (*PFNISR) (void);

PFNISR pfnISR[16] = {
   NULL,
   NULL,
   NULL,
   (PFNISR) AutodetectISR3,
   (PFNISR) AutodetectISR4,
   (PFNISR) AutodetectISR5,
   NULL,
   (PFNISR) AutodetectISR7,
   NULL,
   (PFNISR) AutodetectISR9,
   (PFNISR) AutodetectISR10,
   (PFNISR) AutodetectISR11,
   (PFNISR) AutodetectISR12,
   NULL,
   (PFNISR) AutodetectISR14,
   (PFNISR) AutodetectISR15
};

PRINTF printf2 = (PRINTF) _printf;

void AutodetectISR(unsigned uiIRQ)
{
   cli();

// First, check if the interrupt is from an MPU-401
// if not, set carry and return.
// This is where we detect for spurious interrupts (SI).  Note that it's
// possible for an MPU-401 to generate an SI before it's acknowledged the
// reset, and then generate another interrupt for the ack.  In this case,
// the SI will cause uiFlakyIRQ to be set, and the second will be correctly
// detected.  So the user will get a message saying that IRQ xx is spurious,
// and that the same IRQ has been autodetected.  This might be confusing,
// but the user should be made aware of flaky hardware.

   if (inp(usAutoStatPort) & DSR) {
//      printf2("MPU: IRQ %ui not from MPU port %x\n", uiIRQ, usAutoStatPort - 1);
      usFlakyIO = usAutoStatPort;
      uiFlakyIRQ = uiIRQ;
      stc();
      return;
   }

// Okay, so now we have our interrupt.  Unset the IRQ.
// If this fails, something strange has happened, so just abort
   if (DevHelp_UnSetIRQ(uiIRQ)) {
//      printf2("MPU: UnSetIRQ failed for IRQ %ui and port %x\n", uiIRQ, usAutoStatPort - 1);
      stc();
      return;
   }

// Now check to see if we were actually testing for this IRQ
// If not, then something else strange has happened, so abort
   if (!pfnISR[uiIRQ]) {
//      printf2("MPU: IRQ %ui not available\n", uiIRQ);
      stc();
      return;
   }

   uAutoIRQ = uiIRQ;          // Save the IRQ in our array
   DevHelp_UnSetIRQ(uiIRQ);   // Unset the IRQ
   pfnISR[uiIRQ] = 0;         // Mark this IRQ as taken

   DevHelp_EOI(uiIRQ);
   clc();
}

/* Function: AutodetectIRQ
   Input: The I/O address to detect
   Output: The IRQ of the given port, 0 is no IRQ found
   Assumes: An MPU-401 exists at that base I/O address defined by that port
   Purpose: Determines the IRQ level of the particular MPU-401 specified
            by the port number.  If found, the IRQ level is returned,
            otherwise, zero is returned.  This means that this driver does
            not support MPU-401's at IRQ 0.
   Note: This driver will check the IRQ's with RM if fNoRMIRQ is set to false.
*/
unsigned AutodetectIRQ(USHORT usDataPort)
{
   unsigned i;
//   static char local_irq_resource[sizeof(IRQ_RESOURCE)];
//   IRQ_RESOURCE *ppres, *mem = (IRQ_RESOURCE *) local_irq_resource;
   unsigned int fIRQSet = FALSE;

   for (i = 0; i<16; i++) {
      if (pfnISR[i]) {
         if (!fNoRMIRQ) {
            if (!gpadpt->CheckIRQ(i, RS_PCI_INT_NONE, usRMIRQFlag)) {
               pfnISR[i] = 0;
               continue;
            }
         }
         if (DevHelp_SetIRQ((NPFN) pfnISR[i], i, fMCA ? 1 : 0)) {
            pfnISR[i] = 0;
            continue;
         }
         fIRQSet = TRUE;
      }
   }

   if (!fIRQSet)     // Did we set at least one IRQ?
      return 0;      // Nope, so there's nothing left to test!

   usFlakyIO = 0;
   uiFlakyIRQ = 0;
   uAutoIRQ = 0;
   usAutoStatPort = usDataPort + 1;

   if (!MPUcommand(usDataPort, 0xFF)) {
      for (i = 0; i<30000; i++) {
         if (uAutoIRQ) break;
         iodelay(1);
      }
   }

   for (i = 0; i<16; i++)
      if (pfnISR[i])
         DevHelp_UnSetIRQ(i);

// Here we check if we got spurious interrupts.  If so, print a message and
// mark the interrupt as unavailable.  We're assuming that a spurious
// interrupt can't be from an MPU, unless it has already been detected
// as belonging to an IRQ (see the comments in AutodetectISR)
   if (uiFlakyIRQ) {
      iprintf(szFlakyIRQ, uiFlakyIRQ, usFlakyIO);
      pfnISR[uiFlakyIRQ] = 0;
      usFlakyIO = 0;
      uiFlakyIRQ = 0;
   }

   return uAutoIRQ;
}

/* Function: VerifyIRQ
   Input: The I/O address to check, the IRQ it should be at
   Output: TRUE if the IRQ is correct, FALSE if otherwise or error
   Assumes: An MPU-401 exists at that base I/O address defined by that port
   Purpose: Checks whether the IRQ specified is the IRQ that the
            hardware generates in response to a reset at the specified
            I/O address.
   Note: This driver will check the IRQ with RM if fNoRMIRQ is set to FALSE.
         However, it will immediately un-claimed it.  The claim is made only
         to see if it can be claimed.

*/
int VerifyIRQ(USHORT usDataPort, USHORT usIRQ)
{
   if (!pfnISR[usIRQ])      // Is there an ISR for this IRQ?
      return FALSE;

/*
   if (!fNoRMIRQ) {
      if (!gpadpt->CheckIRQ(usIRQ, RS_PCI_INT_NONE, usRMIRQFlag))
         return FALSE;
   }
*/

   if (DevHelp_SetIRQ((NPFN) pfnISR[usIRQ], usIRQ, fMCA ? 1 : 0))
      return FALSE;

   usFlakyIO = 0;
   uiFlakyIRQ = 0;
   uAutoIRQ = 0;
   usAutoStatPort = usDataPort + 1;

   if (!MPUcommand(usDataPort, 0xFF)) {
      for (unsigned i = 0; i<30000; i++) {
         if (uAutoIRQ) break;
         iodelay(1);
      }
   }

   DevHelp_UnSetIRQ(usIRQ);      // This may be redundant

// Here we check if we got a spurious interrupt.  If so, print a message and
// mark the interrupt as unavailable.  We're assuming that a spurious
// interrupt can't be from an MPU, unless it has already been detected
// as belonging to an IRQ (see the comments in AutodetectISR)

   if (uiFlakyIRQ) {
      iprintf(szFlakyIRQ, uiFlakyIRQ, usFlakyIO);
      pfnISR[uiFlakyIRQ] = 0;
      usFlakyIO = 0;
      uiFlakyIRQ = 0;
   }

   return uAutoIRQ == usIRQ;
}

int CheckPort(USHORT usDataPort)
{
   // If we care that the port is reserved, then check it
   if (!fNoRMIO) {
      if (!gpadpt->CheckPort(usDataPort, 2, usRMIOFlag, usRMWordSize))
         return FALSE;
   }

// at this point, either it's not reserved, or we don't care that it is

   if (fNoCheckIO)
      return TRUE;

   if (!MPUcommand(usDataPort, 0xFF))
      return TRUE;

   return FALSE;
}

int AllocatePort(USHORT usDataPort, USHORT usCL_IRQ)
{
   DEVICE *pdev = new DEVICE(&devicestruct, gpadpt);
   if (!pdev)
      return FALSE;              // out of memory
   if (!pdev->success()) {
      delete pdev;               // RM call failed (should never happen)
      return FALSE;
   }

   PORT_RESOURCE *ppresIO = NULL;

   if (!fNoRMIO) {
      ppresIO = new PORT_RESOURCE(usDataPort, 2, usRMIOFlag, usRMWordSize);
      if (!ppresIO) {
         delete pdev;               // out of memory
         return FALSE;
      }
      if (!pdev->add(ppresIO)) {    // allocate the resource
         HALT;                      // RM call failed. This should never happen,
         delete ppresIO;            //  because we already checked with RM in
         delete pdev;               //  CheckPort
         return FALSE;
      }
   }

   MPU *pmpu = new MPU(usDataPort);
   if (!pmpu) {
      if (ppresIO)                  // out of memory
         delete ppresIO;
      delete pdev;
      return FALSE;
   }

   // If we have RM exclusive access to the I/O port, then let's remember that
   // We'll need this information in vdd.cpp
   if (ppresIO && (usRMIOFlag == RS_IO_EXCLUSIVE))
      pmpu->setExclusive();

   IRQ_RESOURCE *ppresIRQ = NULL;

   if (!usCL_IRQ) {     // Has the user specified an IRQ?
      if (fNoCheckIRQ)  // No. Did he say not to check for one?
         return TRUE;   // Yes. So exit.

      unsigned iIRQ = AutodetectIRQ(usDataPort);   // Look for an IRQ
      if (!iIRQ)           // Did we find one?
         return TRUE;      // No. So exit

      if (!fNoRMIRQ) {
         ppresIRQ = new IRQ_RESOURCE(iIRQ, RS_PCI_INT_NONE, usRMIRQFlag);
         if (!ppresIRQ) {
            delete pdev;               // out of memory
            return FALSE;
         }
         if (!pdev->add(ppresIRQ)) {   // allocate the resource
            HALT;
            if (ppresIO)               // RM call failed.
               delete ppresIO;         // This should never happen, because
            delete ppresIRQ;           // AutodetectIRQ checks for these things
            delete pdev;
            return FALSE;
         }
      }

      IRQ *pirq = new IRQ(iIRQ, fMCA);
      if (!pirq) {
         delete ppresIRQ;
         if (ppresIO)
            delete ppresIO;
         delete pdev;
         return FALSE;
      }
      pmpu->setIRQ(pirq);
      MPUISR *pisr = new MPUISR(pirq, pmpu);
      if (!pisr) {
         delete pirq;
         delete ppresIRQ;
         if (ppresIO)
            delete ppresIO;
         delete pdev;
         return FALSE;
      }

      // If we have RM exclusive access to the IRQ, then let's remember that
      // We'll need this information in vdd.cpp
      if (ppresIRQ && (usRMIRQFlag == RS_IRQ_EXCLUSIVE))
         pirq->setExclusive();

      return TRUE;
   }

// At this point, the user has specified an IRQ on the command-line
// First we check if he wants to use TIMER0

   if (usCL_IRQ == -1) {
      if (pfnTimer) {
         TIMER0 *pirq = new TIMER0;
         pmpu->setIRQ(pirq);
         new MPUISR(pirq, pmpu);
      }
      return TRUE;
   }

// Ok, he doesn't want to use TIMER0

   if (!fNoRMIRQ) {        // Are we supposed to register with RM?
      if (!gpadpt->CheckIRQ(usCL_IRQ, RS_PCI_INT_NONE, usRMIRQFlag)) {
         iprintf(szRMIRQ, usCL_IRQ);
         if (ppresIO) delete ppresIO;
         delete pdev;
         return FALSE;
      }

      ppresIRQ = new IRQ_RESOURCE(usCL_IRQ, RS_PCI_INT_NONE, usRMIRQFlag);
      if (!ppresIRQ) {
         if (ppresIO)
            delete ppresIO;
         delete pdev;
         return FALSE;
      }

      if (!pdev->add(ppresIRQ)) {
         HALT;                      // We should never get here, since
         if (ppresIO)               //  the resource was already cleared
            delete ppresIO;         //  via the CheckIRQ call
         delete ppresIRQ;
         delete pdev;
         return FALSE;
      }
   }

// At this point, the IRQ has been registered with RM if necessary
// All we need to do now is verify it if necessary, and then
// create the IRQ object

   if (!fNoCheckIRQ) {
      if (!VerifyIRQ(usDataPort, usCL_IRQ)) {
         iprintf(szNOIRQ);
         if (ppresIRQ) delete ppresIRQ;
         if (ppresIO) delete ppresIO;
         delete pdev;
         return FALSE;
      }
   }

   IRQ *pirq = new IRQ(usCL_IRQ, fMCA);
   if (!pirq) {
      if (ppresIRQ) delete ppresIRQ;
      if (ppresIO) delete ppresIO;
      delete pdev;
      return FALSE;
   }
   pmpu->setIRQ(pirq);
   new MPUISR(pirq, pmpu);

   // If we have RM exclusive access to the IRQ, then let's remember that
   // We'll need this information in vdd.cpp
   if (ppresIRQ && (usRMIRQFlag == RS_IRQ_EXCLUSIVE))
      pirq->setExclusive();

   return TRUE;
}

int FindPorts(void)
{
   unsigned int fFound = FALSE;
   USHORT usDataPort;

   iprintf(szCheckingPorts);

   for (unsigned iPort = 0; iPort<NUM_AUTODETECT; iPort++) {
      usDataPort = ausAutoDetectPort[iPort];
      iprintf(iPort ? ", %x" : "%x", usDataPort);
      if (CheckPort(usDataPort)) {
         if (!AllocatePort(usDataPort, 0))
            return FALSE;
         if (fOnlyOne) {
            iprintf(".\n");
            return TRUE;
         }
         fFound = TRUE;
      }
   }

// The MCA Roland MPU-401 card uses address 0x1330 instead of 0x300
// for the alternate address.

   if (fMCA) {
      iprintf(", 1330");
      if (CheckPort(0x1330))
         if (!AllocatePort(0x1330, 0))
            return FALSE;
         fFound = TRUE;
   }

   iprintf(".\n");
   return fFound;
}

// For resource management

DRIVERSTRUCT driverstruct =
{
   (PSZ) "MPU401.SYS",                             /* DrvrName                */
   (PSZ) "MPU-401 Audio Card Driver",              /* DrvrDescript            */
   (PSZ) "Theta Band Software",                    /* VendorName              */
   CMVERSION_MAJOR,                                /* MajorVer                */
   CMVERSION_MINOR,                                /* MinorVer                */
   1999, 02, 01,                                   /* Date                    */
   DRF_STATIC,                                     /* DrvrFlags               */
   DRT_AUDIO,                                      /* DrvrType                */
   0,                                              /* DrvrSubType             */
   NULL                                            /* DrvrCallback            */
};

ADAPTERSTRUCT adapterstruct =
{
  (PSZ) "MPU-401 MIDI",                /* AdaptDescriptName; */
  AS_NO16MB_ADDRESS_LIMIT,             /* AdaptFlags;        */
  AS_BASE_MMEDIA,                      /* BaseType;          */
  AS_SUB_MM_AUDIO,                     /* SubType;           */
  AS_INTF_GENERIC,                     /* InterfaceType;     */
  AS_HOSTBUS_ISA,                      /* HostBusType;       */
  AS_BUSWIDTH_16BIT,                   /* HostBusWidth;      */
  NULL                                 /* pAdjunctList;      */
};

void MPU401_StrategyInit(PREQPACKET prp)
{
   static PRINTF_ATTACH_DD DDTable;
   int i;
   USHORT lid;

   Device_Help = prp->s.init_in.ulDevHlp;

   prp->s.init_out.usCodeEnd = 0;
   prp->s.init_out.usDataEnd = 0;
   prp->usStatus = RPDONE | RPERR | RPGENFAIL;

   MonoInit();

   if (!DevHelp_AttachDD((NPSZ) "PRINTF$ ", (NPBYTE) &DDTable))
      if (OFFSETOF(DDTable.pfn)) {
         printf2 = (PRINTF) DDTable.pfn;
         fVerbose = TRUE;
         iprintf(szPrintf);
      }

   if (!DevHelp_GetLIDEntry(0x10, 0, 1, &lid)) {
      DevHelp_FreeLIDEntry(lid);
      fMCA = TRUE;
      usRMWordSize = 16;
   }

   // Parse the command line
   if (!GetParms(prp->s.init_in.szArgs)) {
      iprintf(szBadParam);
      return;
   }

   if (szCL_DevName[0] != ' ')               // Was /N not specified?
      FindUniqueHeader();

   iprintf(szInit);

   // Check for incompatible parameter combos
   if (!fParamsSpecified && fNoCheckIO) {    // /O:NOCHECKIO specified w/ autodetection?
      iprintf(szNoIOandAuto);                // Can't do that!
      return;
   }

   if (!fParamsSpecified && fNoCheckIRQ) {   // /O:NOCHECKIRQ specified w/ autodetection?
      iprintf(szNoIRQandAuto);               // Can't do that!
      return;
   }

   if (fInt3) {
      fVerbose = TRUE;
      int3();
   }

   if (!fNoTimer0) {
      if (DevHelp_AttachDD((NPSZ) "TIMER0$ ", (NPBYTE) &DDTable))
         iprintf(szNoTimer0);
      else
         pfnTimer = (PTMRFN) DDTable.pfn;
   }

   USHORT __far *pusKernelVar;
   if (!DevHelp_GetDOSVar(DHGETDOSV_SMPACTIVE, 0, (PPVOID) &pusKernelVar)) {
      if (*pusKernelVar) {
         iprintf(szSMP);
         fSMP = TRUE;
      }
   }

   if (fMCA)
      iprintf(szMCA);

   if (szCL_DevName[0] != ' ') {              // Was a valid device name specified?
      memcpy(phdr[0].abName,szCL_DevName,8);  // yes, copy it to the dev header
#ifdef DEBUG
      memcpy(phdr[1].abName,szCL_DevName,8);  // and update the TRACE header, too
      phdr[1].abName[0]++;                    // Differentiate it by one letter
#endif
   }

#ifdef EXPIRE
   DATETIME dt;
   DosGetDateTime(&dt);
   if (dt.year < usBuildYear) {
      iprintf(szExpired);
      DosBeep(1500, 2000);
      return;
   }
   if (dt.month < usBuildMonth) {
      iprintf(szExpired);
      DosBeep(1500, 2000);
      return;
   }
   if (dt.day < usBuildDay) {
      iprintf(szExpired);
      DosBeep(1500, 2000);
      return;
   }
   if (dt.year > usBuildYear)
      dt.month += 12;
   if (dt.month > (usBuildMonth + 2)) {
      iprintf(szExpired);
      DosBeep(1500, 2000);
      return;
   }
#endif

   DRIVER *pdrv = new DRIVER(&driverstruct);
   ASSERT(pdrv);
   if (!pdrv || !pdrv->success()) {
      prp->usStatus = RPDONE | RPERR | RPINITFAIL;
      prp->s.init_out.usCodeEnd = 0;
      prp->s.init_out.usDataEnd = 0;
      iprintf(szRMDriver);
      return;
   }

   gpadpt = new ADAPTER(&adapterstruct, pdrv);
   ASSERT(gpadpt);
   if (!gpadpt || !gpadpt->success()) {
      delete pdrv;
      prp->usStatus = RPDONE | RPERR | RPINITFAIL;
      prp->s.init_out.usCodeEnd = 0;
      prp->s.init_out.usDataEnd = 0;
      iprintf(szRMAdapter);
      return;
   }

   llMPU.init();
   llStreams.init();
   llEvents.init();

   if (fParamsSpecified) {                         // params specified?
      for (i = 0; i<MAX_MPU401; i++)
         if (ausCL_IRQ[i] && !ausCL_BaseIO[i]) {   // then check if the IRQ
            iprintf(szIRQnoPort);                  //  was specified without
            delete pdrv;                           //  a base port
            return;
         }
      for (i = 0; i<MAX_MPU401; i++)           // okay, lets verify everything
         if (ausCL_BaseIO[i]) {
            if (!CheckPort(ausCL_BaseIO[i])) {    // oops, port doesn't exist
               iprintf(szNoPort);
               delete pdrv;
               return;
            }
            if (!AllocatePort(ausCL_BaseIO[i], ausCL_IRQ[i])) {
               iprintf(szAllocPort, ausCL_BaseIO[i], ausCL_IRQ[i]);
               delete pdrv;
               return;
            }
         }
   } else {                                  // no params - do full autodetect
      if (!FindPorts()) {                             // so did we find any 401's?
         iprintf(szNoMPU);
         prp->usStatus = RPDONE | RPERR | RPINITFAIL; // no, so exit quietly
         prp->s.init_out.usCodeEnd = 0;
         prp->s.init_out.usDataEnd = 0;
         delete pdrv;
         return;
      }
   }

   if (fVerbose)                            // Display list of 401's found
      i = 1;
      for (MPU *pmpu = (MPU *) (llMPU.Head()); pmpu; pmpu = (MPU *) (pmpu->plNext)) {
         if (pmpu->irq()) {
            if (pmpu->irq()->irq())
               iprintf(szFoundMPUIRQ, i, pmpu->port(), pmpu->irq()->irq());
            else
               iprintf(szFoundMPUIRQ0, i, pmpu->port());
         } else
            iprintf(szFoundMPU, i, pmpu->port());
         i++;
      }

//   InitStream();
   InitMidiStream(iCL_Resolution);

   if (!RegisterPDD((PFN) PDDEntryPoint)) {
      iprintf(szNoVdd);
      return;
   }

#ifdef DEBUG
   memset(&status, 0, sizeof(STATUS));

   status.usLength = sizeof(STATUS);
   status.fSMP = fSMP;
   status.fMCA = fMCA;
   status.usNumMPUs = i - 1;
   pmpu = (MPU *) llMPU.Head();
   for (i=0; i<status.usNumMPUs; i++) {
      if (!pmpu)
         break;
      status.ausIOPorts[i] = pmpu->port();
      if (pmpu->irq())
         status.ausIRQs[i] = pmpu->irq()->irq();
      else
         status.ausIRQs[i] = -1;
      pmpu = (MPU *) (pmpu->plNext);
   }
   status.fQuietInit = fQuietInit;
#endif

   prp->usStatus = RPDONE;
   prp->s.init_out.usCodeEnd = (USHORT) end_of_text;     // set end of code
   prp->s.init_out.usDataEnd = (USHORT) &end_of_data;
   iprintf(szSuccess);
   printf = printf2;
   fRing3 = FALSE;
}
