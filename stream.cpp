/* STREAM.CPP - IDC communciations with MMPM/2's SSM

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
   10-Jan-97  Timur Tabi   Added Register and DeRegister
                           Added FindFile
                           Streams now created at StrategyOpen time
                           Constructor now takes file handle
                           Added volume support
   27-Mar-97  Timur Tabi   delete was using hstream instead of ulSysFileNum
   30-Mar-97  Timur Tabi   ~STREAM now calls DeRegister, to be safe
                           FindStream now checks if ulSysFileNum != 0
                           Removed printf()'s
*/

#define INCL_NOPMAPI
#define INCL_DOSERRORS            // for ERROR_INVALID_FUNCTION
#include <os2.h>
#include <os2me.h>

#include <include.h>
#include <devhelp.h>

#include "trace.h"
#include "stream.hpp"
#include "event.hpp"

/*
The following definition, MAX_CLASS_SIZE, must be equal to or greater than
the size, in bytes, of the largest class derived from class STREAM.  For
example, if there were two derived classes, MIDISTREAM and WAVESTREAM, this
value would be equal to (or greater than) max(sizeof(MIDISTREAM),
sizeof(WAVESTREAM)). This information is obtained by looking at the listing
file.

This awkward hack is used to allow the base class, STREAM, to allocate and
manage the buffers that hold the STREAM-based objects.  The only alternative
is to create a full-blown heap and custom versions of malloc() and free().
Note that the code for such a beast can be found in malloc.c of component
MIDI.

The value chosen should be rounded up to the nearest multiple of 4, to allow
for dword-aligned memory addressing.
*/

#define MAX_CLASS_SIZE     80
#define MAX_STREAMS        16    // max # streams. See also MAX_HANDLES in SSM_IDC.CPP

BYTE bMasterVolume = 128;        // The master volume, 128 is maximum (100%)
unsigned uNumStreams = 0;        // The number of streams currently open

LINKLIST llStreams;

void STREAM::ReturnBuffer(unsigned u)
{
   static SHD_REPORTINT shdri = {       // structure used to return buffers to SHD
      SHD_REPORT_INT,
      0, 0,
      SHD_WRITE_COMPLETE
   };

   shdri.hStream=hstream;
   shdri.pBuffer=apbuf[u];
   shdri.ulStatus=auBufLen[u];
   shdri.ulStreamTime=ulCurrentTime;
   pfnSHD(&shdri);
}

void STREAM::ReturnBuffers(void)
{
   while (uHead != uTail) {
      ReturnBuffer(uHead);
      uHead = (uHead+1) & (MAX_BUFFERS-1);
   }

   uBufIdx=0;
}

void STREAM::ProcessEvents(void)
{
   while (peventNext)
      if (peventNext->ulNextTime <= ulCurrentTime)
         peventNext->Report();
      else
         break;
}

ULONG STREAM::Control(PDDCMDCONTROL p)
{
   switch (p->ulCmd) {
      case DDCMD_START:
         Trace(TRACE_DDCMD_CNTRL_START, 0);
         if (!Start())
            return ERROR_START_STREAM;
         fPaused = FALSE;
         fActive = TRUE;
         break;
      case DDCMD_STOP:
         Trace(TRACE_DDCMD_CNTRL_STOP, 0);
         Stop();
         ReturnBuffers();
         fPaused = FALSE;
         fActive = FALSE;
         p->pParm = &ulCurrentTime;
         p->ulParmSize = sizeof(ULONG);
         break;
      case DDCMD_PAUSE:
         Trace(TRACE_DDCMD_CNTRL_PAUSE, fPaused);
         if (fPaused) return ERROR_INVALID_SEQUENCE;
         Pause();
         fPaused = TRUE;
         fActive = FALSE;
         p->pParm = &ulCurrentTime;
         p->ulParmSize = sizeof(ULONG);
         break;
      case DDCMD_RESUME:
         Trace(TRACE_DDCMD_CNTRL_RESUME, fPaused);
         if (!fPaused) return ERROR_INVALID_SEQUENCE;
         Resume();
         fPaused = FALSE;
         fActive = TRUE;
         break;
      case DDCMD_ENABLE_EVENT: {
         Trace(TRACE_DDCMD_CNTRL_EVENT_ON, p->hEvent);
         EVENT *pevent = FindEvent(p->hEvent);
         if (pevent)
            pevent = new (pevent) EVENT(this,p->hEvent,(PCONTROL_PARM) p->pParm);
         else
            pevent = new EVENT(this,p->hEvent,(PCONTROL_PARM) p->pParm);
         if (!pevent)
            return ERROR_TOO_MANY_EVENTS;
         break;
      }
      case DDCMD_DISABLE_EVENT: {
         Trace(TRACE_DDCMD_CNTRL_EVENT_OFF, p->hEvent);
         EVENT *pevent=FindEvent(p->hEvent);
         if (!pevent)
            return ERROR_INVALID_EVENT;
         delete pevent;
         break;
      }
      case DDCMD_PAUSE_TIME:
         Trace(TRACE_DDCMD_CNTRL_PAUSE_TIME, ulCurrentTime);
         fIncrementCounter = FALSE;
         break;
      case DDCMD_RESUME_TIME:
         Trace(TRACE_DDCMD_CNTRL_RESUME_TIME, ulCurrentTime);
         fIncrementCounter = TRUE;
         break;
   }
   return 0;
}

ULONG STREAM::Write(PSTREAMBUF pbuf, unsigned uLength)
{
   if (((uTail+1) & (MAX_BUFFERS-1)) == uHead)
      return ERROR_TOO_MANY_BUFFERS;

   apbuf[uTail]=pbuf;
   auBufLen[uTail]=uLength;
   uBufIdx=0;
   uTail = (uTail+1) & (MAX_BUFFERS-1);

   return 0;
}

void STREAM::Register(PDDCMDREGISTER p)
{
   ASSERT(p);
   hstream = p->hStream;
   pfnSHD = (PFN_SHD) p->pSHDEntryPoint;
   p->ulAddressType = ADDRESS_TYPE_VIRTUAL;
}

void STREAM::DeRegister(void)
{
   ASSERT(hstream);
   Stop();
   hstream = 0;
   pfnSHD = NULL;
}

STREAM::STREAM(ULONG _ulSysFileNum)
   : LINKABLE(&llStreams)
{
   ASSERT(_ulSysFileNum);
   ASSERT(uNumStreams != -1);
   ulSysFileNum = _ulSysFileNum;
   phw = NULL;

   fPaused = FALSE;
   fIncrementCounter = TRUE;
   uHead = 1;
   uTail = 1;
   ulCurrentTime = 1;
   bVolume = 128;

   hstream = 0;
   pfnSHD = NULL;
   uNumStreams++;
}

STREAM::~STREAM(void)
{
   ASSERT(uNumStreams);

   if (hstream) {
      int3();
      DeRegister();
   }
   uNumStreams--;
}

STREAM *FindStream(HSTREAM hstream)
{
   STREAM *pstream = (STREAM *) llStreams.Head();

   while (pstream) {
      if ( (pstream->hstream == hstream) && pstream->ulSysFileNum)
         return pstream;
      pstream = (STREAM *) pstream->plNext;
   }

   return NULL;
}

STREAM *FindFile(ULONG ulSysFileNum)
{
   STREAM *pstream = (STREAM *) llStreams.Head();

   while (pstream) {
      if (pstream->ulSysFileNum == ulSysFileNum)
         return pstream;
      pstream = (STREAM *) pstream->plNext;
   }

   return NULL;
}

