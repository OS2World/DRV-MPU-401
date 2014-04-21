/* STRATEGY.CPP - Strategy entry points

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
   01-Jan-96  Timur Tabi   Added support for long Type A names
   18-Apr-96  Timur Tabi   Added SELECTOF checks
   09-Jan-97  Timur Tabi   Converted to .cpp file
                           Added Open and Close
   02-Feb-97  Timur Tabi   Split InstanceName into ShortName and LongName
                           Removed MPU401_StrategyDeinstall
   30-Mar-97  Timur Tabi   Close was calling FindStream, not FindFile
                           Remove printf() calls
*/

#define INCL_NOPMAPI
#define INCL_DOSINFOSEG
#include <os2.h>
#include <audio.h>
#include <os2medef.h>

#include <include.h>
#include <devhelp.h>
#include "..\wpddlib\strategy.h"
#include "end.h"

#include "trace.h"
#include "timer0.hpp"
#include "mpu401.hpp"
#include "isr.hpp"
#include "idc.h"
#include "midistrm.hpp"

#include "..\midi\midi_idc.h"

#include "..\printf\printf.h"


// The address of the function to call for Type A de-registration
PFNMIDI_DEREGISTER pfnDeregister;

// TRUE if the Type A registration names should include base address and IRQ
unsigned int fLongNames = FALSE;

void WriteHex(char *sz, USHORT us)
{
   static char abDigits[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

   *sz++ = abDigits[us >> 12];
   *sz++ = abDigits[(us >> 8) & 15];
   *sz++ = abDigits[(us >> 4) & 15];
   *sz = abDigits[us & 15];
}

void WriteDec(char *sz, BYTE b)
{
   *sz++ = b > 9 ? '1' : '0';
   *sz = (BYTE) ((b % 10) + '0');
}

// szInstanceName is the name passed to RTMIDI during Type A registration.
// This driver supports multiple registrations (up to 9), where each
// registration represents a different MPU-401 port.  For each registration,
// szInstanceName is modified.  This makes the code a little more complicated,
// but it saves space

// szInstance[9] is the first '0', after the #-sign.  This indicates which
// MPU-401 port.  The first is numbered 1, the second is 2, and so on.

// szInstance[10] is set to ASCII 0 if long name support is off.  Long name
// support means that the instance name also contains the base I/O address
// and IRQ (if it exists).  By setting index 10 to 0, we are effectively
// truncating the string before the left parenthesis.

// szInstance[16] is the offset of the base I/O address.  If long name support
// is enabled, function WriteHex is called with the &szInstance[12] and the
// value of the base I/O address.

// szInstance[26] is the offset of the IRQ.  If long name support is enabled,
// and an IRQ function WriteDec is called to write the IRQ to this location.

void MPU401_StrategyInitComplete(PREQPACKET prp)
{
   static char szShortName[] = "MPU-401 #0";
   static char szLongName[] = "MPU-401 (I/O=0000, IRQ=00)";
   static MIDI_ATTACH_DD DDTable;   // it's got to be in DS, so make it static
   MIDI_REGISTER reg;
   MIDIREG_TYPEA regA;
   int i;
   USHORT usRC;

   void __far *p = (void __far *) &pulTimer;
   if (pfnTimer)
      pfnTimer(TMR0_GETTMRPTR, (ULONG) p, 0);

   if (DevHelp_AttachDD((NPSZ) "MIDI$   ", (NPBYTE) &DDTable))
      return;

   reg.usSize=sizeof(reg);
   DDTable.pfn(&reg);

// The following code is used to see if something's went wrong with the
// call to MIDI.SYS.  It checks to see if the selectors of the entry points
// are equal.  Since all MIDI.SYS function entry points are in the same
// segment, they must all have the same selector.  If they don't, then the
// call failed.
// This check is necessary because the original MIDI.SYS only filled in the
// reg structure if the usSize field was set to exactly 12.  It should have
// check to see if it was at least 12.  If this check isn't made, and our
// driver connects to the old MIDI.SYS, then the call to reg.pfnRegisterA()
// will cause a Trap D.
   if (SELECTOROF(reg.pfnRegisterA) != SELECTOROF(DDTable.pfn))
      return;

   regA.usSize = sizeof(regA);

// the string itself changes, but the pointer to it doesn't
   regA.in.pszInstanceName=fLongNames ? szLongName : szShortName;
   regA.in.pfnOpen=Open;
   regA.in.pfnClose=Close;
   regA.in.pfnRecvString=RecvString;      // Required
   regA.in.pfnRecvByte=RecvByte;          // Required

// This is also an MMPM driver, so we don't want to support the MMPM bridge
   regA.in.pfnIOCtl=NULL;

// Remember, "input" means that we can receive data from RTMIDI, and
// "output" means that we can send data to RTMIDI.  We can always write to
// an MPU-401 port, so we always set the MIDICAPSA_INPUT flag.  If
// we also have an IRQ for that port, then it means that we can send data
// to RTMIDI, so we set the MIDICAPSA_OUTPUT flag as well.

// If there is no IRQ, then we have to change the string from
// "(I/O=0000, IRQ=00)" to just "(I/O=0000)".  This means that the comma
// must be replaced with a right parenthesis and an ASCII 0 must be written
// to the next byte (to truncate the string).  To be on the safe side, if
// there is an IRQ available, we write the ", " back, in case they were
// erased in a previous pass of the loop.

   for (MPU *pmpu = (MPU *) (llMPU.Head()); pmpu; pmpu = (MPU *) (pmpu->plNext)) {
         regA.in.flCapabilities = MIDICAPSA_INPUT;        // we can do input
         if (fLongNames)
            WriteHex(&szLongName[13], pmpu->port());
         if (pmpu->irq()) {                                    // do we have an IRQ
            regA.in.flCapabilities |= MIDICAPSA_OUTPUT;   // yes, so we can do output also
            WriteDec(&szLongName[23], (BYTE) pmpu->irq()->irq());
            szLongName[17] = ',';
            szLongName[18] = ' ';
         } else {
            szLongName[17] = ')';
            szLongName[18] = 0;
         }

         regA.in.usDevId = (USHORT) pmpu;   // The device ID is the ptr to the MPU object
         szShortName[9] = (BYTE) (i + '1');

         usRC = reg.pfnRegisterA(&regA);
         if (usRC) {
            prp->usStatus |= RPERR | RPGENFAIL;
            return;
         }
         pmpu->setHandle(regA.out.ulHandle);   // save the 32-bit handle
   }

// We are assuming that regA.out.pfnSendByte and regA.out.pfnDeregister
// are the same for all registrations.  And we're also assuming that after
// we're doing registering all of our devices, these fields in our regA
// structure haven't been trashed.

   pfnSendByte = regA.out.pfnSendByte;
   pfnDeregister = regA.out.pfnDeregister;
}

// Althought the STREAM->MIDISTREAM class heirarchy implies that there is
// generic support for any kind of STREAM object (e.g. WAVESTREAM) and that
// the code has built-in support for easily adding additional STREAM types,
// such is not the case.  This sample is riddled with code that assumes
// a MIDISTREAM object.  After all, this is a MIDI-only device driver.  One
// example is the fact that the mode (SHORT sMode) of the stream is not
// stored in the STREAM objects.  Another example is MPU401_StrategyOpen.
// Here, when a file is opened, we immediately create a MIDISTREAM object.
// Normally, we should create a plain old STREAM object, and the replace
// it with a MIDISTREAM (or whatever) object in the IoctlAudioInit routine.
// Instead, we just verify that MMPM/2 wants to open us for MIDI.  If not,
// then we return an error.  When MMPM/2 issues a close, we will then delete
// the MIDISTREAM object.

void MPU401_StrategyOpen(PREQPACKET prp)
{
   extern unsigned int fInUse;
   Trace(TRACE_STRATEGY_OPEN, prp->s.open_close.usSysFileNum);

   if (fInUse) {
      prp->usStatus |= RPERR | RPGENFAIL;
      return;
   }

   MIDISTREAM *pstream = new MIDISTREAM(prp->s.open_close.usSysFileNum, (MPU *) llMPU.Head());
   if (!pstream)
      prp->usStatus |= RPERR | RPGENFAIL;
}

void MPU401_StrategyClose(PREQPACKET prp)
{
   Trace(TRACE_STRATEGY_CLOSE, prp->s.open_close.usSysFileNum);

   MIDISTREAM *pstream = (MIDISTREAM *) FindFile(prp->s.open_close.usSysFileNum);
   if (pstream)
      delete pstream;
   else
      prp->usStatus |= RPERR | RPGENFAIL;
}

void MPU401_StrategyInit(PREQPACKET prp);
void MPU401_StrategyIoctl(PREQPACKET prp);

extern "C" void __far MPU401_StrategyHandler(PREQPACKET prp);
#pragma aux MPU401_StrategyHandler parm [es bx];

extern "C" void __far MPU401_StrategyHandler(PREQPACKET prp)
{
   prp->usStatus = RPDONE;

   switch (prp->bCommand) {
      case STRATEGY_INIT:
         MPU401_StrategyInit(prp);
         break;
      case STRATEGY_OPEN:
         MPU401_StrategyOpen(prp);
         break;
      case STRATEGY_CLOSE:
         MPU401_StrategyClose(prp);
         break;
      case STRATEGY_GENIOCTL:
         MPU401_StrategyIoctl(prp);
         break;
      case STRATEGY_INITCOMPLETE:
         MPU401_StrategyInitComplete(prp);
         break;
      default:
         prp->usStatus = RPDONE | RPERR | RPGENFAIL;
   }
}


