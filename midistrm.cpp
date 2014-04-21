/* MIDISTRM.CPP - derived stream class for MIDI data

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   24-Jan-96  Timur Tabi   Creation
   20-Jan-97  Timur Tabi   Added volume control support
                           Removed destructor
                           Moved stop() code to superclass
*/

#define INCL_NOPMAPI
#define INCL_DOSERRORS            // for ERROR_INVALID_FUNCTION
#include <os2.h>
#include <os2me.h>
#include <audio.h>                // for #define MIDI

#include <include.h>
#include <devhelp.h>
#include "..\printf\printf.h"
#include "iprintf.h"
#include "iodelay.h"
#include "trace.h"
#include "timer0.hpp"

#include "midistrm.hpp"
#include "event.hpp"
#include "mpu401.hpp"

extern "C" unsigned uiMilliseconds=31;    // milliseconds per tick
extern "C" unsigned uiMicroseconds=250;   // residual microseconds per tick
unsigned long ulMicroseconds=31250;       // = uiMilliseconds * 1000 + uiMicroseconds

unsigned uPort;               // the MPU-401 port number to use for all streams

unsigned int fTimerActive = FALSE;     // TRUE if the timer is running

MIDISTREAM *pstreamCurrent=NULL;  // pointer to stream currently being processed

void __far __loadds MPU401_Timer0Hook(void)
{
   if (pstreamCurrent)
      pstreamCurrent->Process();
}

void __far __loadds __saveregs MPU401_SysTimerHook(void)
{
   pusha();

   if (pstreamCurrent)
      pstreamCurrent->Process();

   popa();
}

/*
int MIDISTREAM::InitDevice(void)
{
   Trace(TRACE_MIDISTREAM_INIT, 0);

   return phw->init();
}
*/

virtual int MIDISTREAM::Start(void)
{
   Trace(TRACE_MIDISTREAM_START, fTimerActive);

   if (!phw->start())
      return FALSE;

   if (fTimerActive)          // first check if the timer is already running
      return TRUE;

   if (pfnTimer) {
      if (pfnTimer(TMR0_REG,(ULONG) MPU401_Timer0Hook, uiMilliseconds))
         return FALSE;
      pstreamCurrent = this;
      fTimerActive = TRUE;
      Trace(TRACE_TIMER0_ENABLE, 0);
      return TRUE;
   }

   if (DevHelp_SetTimer((NPFN) MPU401_SysTimerHook))
      return FALSE;

   pstreamCurrent = this;
   fTimerActive = TRUE;
   return TRUE;
}

virtual void MIDISTREAM::Stop(void)
{
   Trace(TRACE_MIDISTREAM_STOP, fTimerActive);

   if (fTimerActive) {
      if (pfnTimer) {
         pfnTimer(TMR0_DEREG,(ULONG) MPU401_Timer0Hook,0);
         Trace(TRACE_TIMER0_DISABLE, 0);
      } else
         DevHelp_ResetTimer((NPFN) MPU401_SysTimerHook);
      fTimerActive = FALSE;
   }

   if (pstreamCurrent == this)
      pstreamCurrent = NULL;

   phw->stop();
}

virtual void MIDISTREAM::Pause(void)
{
   Trace(TRACE_MIDISTREAM_PAUSE, 0);
   phw->silence();
}

virtual void MIDISTREAM::Resume(void)
{
   Trace(TRACE_MIDISTREAM_RESUME, 0);
}

/*           600,000,000 microseconds/10 minutes
  ----------------------------------------------------------   ==   X microseconds/clock
                                      usCPQNnum
    (ulTempo beats/10 min) * ( 24 * ------------- clocks/beat )
                                      usCPQNden


        25,000,000 * usCPQNden
  ==  --------------------------
         ulTempo * usCPQNnum

where
   usCPQNden = ((usCPQN & 0x3F) + 1) * 3
   usCPQNnum = 1                                    if bit 6 of usCPQN is set

      or

   usCPQNden = 1
   usCPQNnum = usCPQN + 1                           if bit 6 is not set

*/

void MIDISTREAM::CalcDelay(void)
{
   ULONG ul;

   if (usCPQN & 0x40) {          // bit 6 is set if it's a denominator
      ul = 25000000 * ((usCPQN & 0x3F) + 1);
      ulPerClock = ul / ulTempo;
      ulPerClock *= 3;
   } else {
      ul = ulTempo * (usCPQN+1);
      ulPerClock = 25000000 / ul;
   }
}

#define STATE_DEFAULT         0     // the initial state
#define STATE_USER_SYSEX      1     // processing a user sysex
#define STATE_SKIP_SYSEX      2     // skipping to the end of the current sysex
#define STATE_SYSEX_2         3     // expecting the 2nd byte of a sysex
#define STATE_SYSEX_3         4     // expecting the 3rd byte of a sysex
#define STATE_SYSEX_4         5     // expecting the 4th byte of a sysex
#define STATE_SYSEX_CMD       6     // expecting the IBM sysex command
#define STATE_TC_LSB          7     // expecting the time compression LSB
#define STATE_TC_MSB          8     // expecting the time compression MSB
#define STATE_DRVR_CNTL_CMD   9     // expecting the driver control command
#define STATE_CPQN_CONTROL    10    // expecting the CPQN control flag
#define STATE_CPQN            11    // expecting the CPQN prescaler
#define STATE_TEMPO_LSB       12    // expecting the tempo LSB
#define STATE_TEMPO_MSB       13    // expecting the tempo MSB
#define STATE_NOTE_ON         14    // expecting the 2nd (note #) byte of a note on
#define STATE_VOLUME          15    // expecting the 3nd (volume) byte of a note on

void MIDISTREAM::Deal(BYTE b)
{
   switch (uState) {
      case STATE_DEFAULT:
         if ((b & 0xF0) == 0x90)       // is this a note-on message?
            uState = STATE_NOTE_ON;
         switch (b) {
            case 0xF8:
               lWait += ulPerClock;
               return;
            case 0xF0:
               uState = STATE_SYSEX_2;
               fRunningStat = FALSE;
               return;
            default:
               if (fRunningStat && (b < 0x80))
                  uState = STATE_VOLUME;
               else
                  fRunningStat = FALSE;
               MPUwrite(uPort,b);
               return;
         }
      case STATE_USER_SYSEX:
         MPUwrite(uPort,b);
         if (b == 0xF7)
            uState = STATE_DEFAULT;
         return;
      case STATE_SKIP_SYSEX:
         if (b == 0xF7) {
            uState = STATE_DEFAULT;
            return;
         }
         return;
      case STATE_SYSEX_2:
         if (b) {
            MPUwrite(uPort,0xF0);
            MPUwrite(uPort,b);
            uState = STATE_USER_SYSEX;
         } else
            uState = STATE_SYSEX_3;
         return;
      case STATE_SYSEX_3:
         if (b) {
            MPUwrite(uPort,0xF0);
            MPUwrite(uPort,0);
            MPUwrite(uPort,b);
            uState = STATE_USER_SYSEX;
         } else
            uState = STATE_SYSEX_4;
         return;
      case STATE_SYSEX_4:
         if (b != 0x3A) {
            MPUwrite(uPort,0xF0);
            MPUwrite(uPort,0);
            MPUwrite(uPort,0);
            MPUwrite(uPort,b);
            uState = STATE_USER_SYSEX;
         } else
            uState = STATE_SYSEX_CMD;
         return;
      case STATE_SYSEX_CMD:
         switch (b) {
            case 0:
            case 2:
            case 4:
            case 5:
            case 6:
               uState = STATE_SKIP_SYSEX;
               return;
            case 1:
               uState = STATE_TC_LSB;
               return;
            case 3:
               uState = STATE_DRVR_CNTL_CMD;
               return;
            default:
               lWait += b * ulPerClock;
               uState = STATE_SKIP_SYSEX;
               return;
         }
      case STATE_TC_LSB:
         usCompValue=b;
         uState = STATE_TC_MSB;
         return;
      case STATE_TC_MSB:
         lWait+=(usCompValue + (b << 7)) * ulPerClock;
         uState = STATE_SKIP_SYSEX;
         return;
      case STATE_DRVR_CNTL_CMD:
         switch (b) {
            case 1:
               uState = STATE_CPQN_CONTROL;
               return;
            case 2:
               uState = STATE_TEMPO_LSB;
               return;
            default:
               uState = STATE_SKIP_SYSEX;
               return;
         }
      case STATE_CPQN_CONTROL:            // ignore it for now
         uState = STATE_CPQN;
         return;
      case STATE_CPQN:
         usCPQN=b;
         uState = STATE_SKIP_SYSEX;
         CalcDelay();
         return;
      case STATE_TEMPO_LSB:
         ulTempo=b;
         uState = STATE_TEMPO_MSB;
         return;
      case STATE_TEMPO_MSB:
         ulTempo += b << 7;
         uState = STATE_SKIP_SYSEX;
         CalcDelay();
         return;
      case STATE_NOTE_ON:
         MPUwrite(uPort,b);
         uState = STATE_VOLUME;
         return;
      case STATE_VOLUME: {
         ULONG ul = bVolume * bMasterVolume;
         ul *= b;
         b = (BYTE) (ul >> 14);
         MPUwrite(uPort,b);
         uState = STATE_DEFAULT;
         fRunningStat = TRUE;
         return;
      }
   }
}

void MIDISTREAM::Process(void)
{
   if (uHead == uTail)        // no buffers to process?
      return;

   if (fPaused)               // is the stream paused?
      return;

   if (fIncrementCounter) {
      uiResidue += uiMicroseconds;
      if (uiResidue >= 1000) {
         ulCurrentTime += uiMilliseconds + 1;
         uiResidue -= 1000;
      } else
         ulCurrentTime += uiMilliseconds;
   }

   ProcessEvents();

   lWait -= ulMicroseconds;

   while (lWait <= 0 && uHead != uTail) {
      Deal(apbuf[uHead][uBufIdx++]);
      if (uBufIdx >= auBufLen[uHead]) {
         ReturnBuffer(uHead);
         uHead = (uHead+1) & (MAX_BUFFERS-1);
         uBufIdx = 0;
      }
   }
}

MIDISTREAM::MIDISTREAM(ULONG _ulSysFileNume, MPU *_pmpu)
   : STREAM(_ulSysFileNume)
{
   phw = _pmpu;
   uiResidue = 0;
   uState = STATE_DEFAULT;
   ulTempo = 1200;
   usCPQN = 0;
   CalcDelay();
   lWait = ulMicroseconds;
   fRunningStat = FALSE;
}

#pragma code_seg ("_inittext");

extern char szResolution[];

extern "C" void InitMidiStream(int iRes)
{
// If the highres timer was found, use the resolution specified
// on the command line
   if (pfnTimer) {
      uiMilliseconds = iRes;
      uiMicroseconds = 0;
      ulMicroseconds = 1000UL * uiMilliseconds;
      iprintf(szResolution, iRes);
   }

// Find the 1st port and use it
   MPU *pmpu = (MPU *) (llMPU.Head());
   uPort = pmpu->port();
}

#pragma code_seg ("_text");

