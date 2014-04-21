/* SSM_IDC.CPP - SSM IDC communication

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Nov-95  Timur Tabi   Creation
   20-Jan-97  Timur Tabi   Removed handle functions: handle[]
                            AddHandle, InitSSM, etc.
                           Streams are now created at Open,
                            and registered at REG_STREAM
*/

#define INCL_NOPMAPI
#define INCL_DOSERRORS            // for ERROR_INVALID_FUNCTION
#include <os2.h>
#include <os2me.h>
#include <audio.h>

#include <include.h>
#include "..\printf\printf.h"

#include "trace.h"
#include "midistrm.hpp"
#include "event.hpp"

#define MAX_HANDLES     16    // should be equal to MAX_STREAMS in STREAM.CPP

extern "C" ULONG __far __loadds __cdecl DDCMD_EntryPoint(PDDCMDCOMMON pCommon)
{
   STREAM *pstream = FindStream(pCommon->hStream);

// printf("MPU: DDCMD %lu stream=%p\n",pCommon->ulFunction,(void __far *) pstream);

   switch (pCommon->ulFunction) {
      case DDCMD_SETUP: {
         Trace(TRACE_DDCMD_SETUP, (ULONG) (void __far *) pstream);
         if (!pstream) return ERROR_INVALID_STREAM;
         PDDCMDSETUP p = (PDDCMDSETUP) pCommon;
         SETUP_PARM __far *psp = (SETUP_PARM __far *) p->pSetupParm;
         pstream->ulCurrentTime = psp->ulStreamTime;
         if (p->ulSetupParmSize > sizeof(ULONG)) {
            fRecurringSupported = TRUE;
            psp->ulFlags = SETUP_RECURRING_EVENTS;
         }
         break;
      }
      case DDCMD_READ:
         Trace(TRACE_DDCMD_READ, (ULONG) (void __far *) pstream);
         return ERROR_INVALID_FUNCTION;
      case DDCMD_WRITE: {
         Trace(TRACE_DDCMD_WRITE, (ULONG) (void __far *) pstream);
         if (!pstream) return ERROR_INVALID_STREAM;
         PDDCMDREADWRITE p = (PDDCMDREADWRITE) pCommon;
         pstream->Write((PSTREAMBUF) p->pBuffer,(unsigned) p->ulBufferSize);
         break;
      }
      case DDCMD_STATUS: {
         Trace(TRACE_DDCMD_STATUS, (ULONG) (void __far *) pstream);
         if (!pstream) return ERROR_INVALID_STREAM;
         PDDCMDSTATUS p = (PDDCMDSTATUS) pCommon;
         PSTATUS_PARM p2 = (PSTATUS_PARM) p->pStatus;
         p2->ulTime = pstream->ulCurrentTime;
         break;
      }
      case DDCMD_CONTROL: {
         if (!pstream) return ERROR_INVALID_STREAM;
         PDDCMDCONTROL p = (PDDCMDCONTROL) pCommon;
         return pstream->Control(p);
      }
      case DDCMD_REG_STREAM: {
         Trace(TRACE_DDCMD_REGISTER, (ULONG) (void __far *) pstream);
         if (pstream)
            return ERROR_HNDLR_REGISTERED;
         pstream = FindFile(((PDDCMDREGISTER) pCommon)->ulSysFileNum);
         if (!pstream)
            return ERROR_STREAM_CREATION;
         pstream->Register((PDDCMDREGISTER) pCommon);
         return 0;
      }
      case DDCMD_DEREG_STREAM:
         Trace(TRACE_DDCMD_DEREG, (ULONG) (void __far *) pstream);
         if (!pstream) return ERROR_INVALID_STREAM;
         pstream->DeRegister();
         break;
      default:
         return ERROR_INVALID_FUNCTION;
   }

   return NO_ERROR;
}

