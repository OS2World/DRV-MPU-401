/* IOCTL.CPP

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
   09-Jan-97  Timur Tabi   Converted to .cpp file
                           Added support for Capabilities IOCtl
                           Replace IoctlFuncs with switch/case statement
                           Added support for volume changes
*/


#define INCL_NOPMAPI
#include <os2.h>

#include <audio.h>
#include <os2medef.h>

#include "trace.h"
#include "midistrm.hpp"
#include "..\wpddlib\strategy.h"

#include <include.h>
#include <devhelp.h>

void MPU401_IoctlAudioInit(PREQPACKET prp)
{
   MCI_AUDIO_INIT __far *p=(MCI_AUDIO_INIT __far *) prp->s.ioctl.pvData;

   Trace(TRACE_STRATEGY_IOCTL_INIT, p->ulOperation);

   if ((p->sMode != MIDI) && (p->sMode != DATATYPE_MIDI) && (p->sMode != 0)) {
      p->sReturnCode = INVALID_REQUEST;
      return;
   }

   p->ulFlags = VOLUME;
   p->sDeviceID = MPU401;

   switch (p->ulOperation) {
      case OPERATION_PLAY:
         p->pvReserved = (VOID FAR *) (ULONG) prp->s.ioctl.usSysFileNum;
         p->sReturnCode = 0;
         break;
      case OPERATION_RECORD:
      case PLAY_AND_RECORD:
         p->sReturnCode = NO_RECORD;
         break;
      default:
         p->sReturnCode = INVALID_REQUEST;
         break;
   }
}

void MPU401_IoctlAudioControl(PREQPACKET prp)
{
   MCI_AUDIO_CONTROL __far *p=(MCI_AUDIO_CONTROL __far *) prp->s.ioctl.pvData;
   MCI_AUDIO_CHANGE __far *pmciac;
   MCI_TRACK_INFO __far *pmciti;

   Trace(TRACE_STRATEGY_IOCTL_CTRL, p->usIOCtlRequest);

   if (p->usIOCtlRequest == AUDIO_CHANGE) {
      pmciac = (MCI_AUDIO_CHANGE __far *) p->pbRequestInfo;
      pmciti = (MCI_TRACK_INFO __far *) pmciac->pvDevInfo;

      if (!(pmciac->lVolume & 0x80000000)) {
         MIDISTREAM *pstream = (MIDISTREAM *) FindFile(prp->s.ioctl.usSysFileNum);
         if (!pstream) {
            p->sReturnCode = INVALID_REQUEST;
            return;
         }
         pstream->bVolume = (BYTE) (((ULONG) pmciac->lVolume) >> 24);
      }
      if (!(pmciti->usMasterVolume & 0x8000))
         bMasterVolume = (BYTE) (pmciti->usMasterVolume >> 8);

      p->sReturnCode = 0;
      return;
   }

   p->sReturnCode = INVALID_REQUEST;
}

void MPU401_IoctlAudioCapability(PREQPACKET prp)
{
   MCI_AUDIO_CAPS __far *p=(MCI_AUDIO_CAPS __far *) prp->s.ioctl.pvData;

   Trace(TRACE_STRATEGY_IOCTL_CAPS, p->ulDataType);

   p->ulCapability = SUPPORT_CAP;

   if (p->ulDataType != DATATYPE_MIDI) {
      p->ulSupport = UNSUPPORTED_DATATYPE;
      return;
   }

   p->ulSupport = SUPPORT_SUCCESS;
   p->ulDataSubType = SUBTYPE_NONE;
   p->ulResourceUnits = 1;
   p->ulResourceClass = 1;
   p->ulFlags = VOLUME;
   p->fCanRecord = FALSE;
}

void MPU401_StrategyIoctl(PREQPACKET prp)
{
   if (prp->s.ioctl.bCategory != AUDIO_IOCTL_CAT) {
      prp->usStatus |= RPERR | RPBADCMD;
      return;
   }

   switch (prp->s.ioctl.bCode) {
      case AUDIO_INIT:
         MPU401_IoctlAudioInit(prp);
         break;
      case AUDIO_CONTROL:
         MPU401_IoctlAudioControl(prp);
         break;
      case AUDIO_CAPABILITY:
         MPU401_IoctlAudioCapability(prp);
         break;
      default:
         prp->usStatus |= RPERR | RPBADCMD;
         return;
   }
}

