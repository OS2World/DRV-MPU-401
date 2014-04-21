/* MPU401.C - MPU-401 I/O commands

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
   16-Nov-95  Timur Tabi   Added support for Quiet Initializations
   24-Apr-96  Timur Tabi   Added autodect support addresses 332, 334, 336
   23-Aug-96  Timur Tabi   Changed MPUcommand to take I/O address instead of index
                           Moved autodetect I/O addresses to init.cpp
   24-Feb-98  Timur Tabi   added fSMP
*/

#define INCL_NOPMAPI
#include <os2.h>
// #include <dos.h>
#include <memory.h>

#include <include.h>
#include <devhelp.h>
#include "trace.h"
#include "iodelay.h"
#include "timer0.hpp"
#include "mpu401.hpp"

#include "..\printf\printf.h"

// USHORT ausDataPort[MAX_MPU401];
// USHORT ausStatPort[MAX_MPU401];

LINKLIST llMPU;
LINKLIST *pllMPU = &llMPU;

PFNMIDI_SENDBYTE pfnSendByte;

extern "C" void __wcpp_4_lcl_register_(void) {}
extern "C" void ___wcpp_4_data_module_dtor_ref(void) {}

unsigned int fQuietInit=FALSE;
// If TRUE, then ignore errors from hardware initialization

unsigned int fSMP = FALSE;
// True if this is an SMP machine

AUDIOHW::AUDIOHW(void)
   : LINKABLE(&llMPU)
{
}

MPU::MPU(USHORT _usDataPort)
{
   usDataPort = _usDataPort;
   usStatPort = _usDataPort + 1;
   fExclusive = FALSE;

   ulHandle = 0;
   pirq = NULL;
}

#define TIMEOUT   10000

int MPU::read(void)
{
   unsigned i;

   for (i=0; i<TIMEOUT; i++) {
      cli();
      if (!(inp(usStatPort) & DSR)) {
         i=inp(usDataPort);
         sti();
//         Trace(TRACE_MPUHW_READ, i);
         return i;
      }
      sti();
      iodelay(1);
   }
   Trace(TRACE_MPUHW_READ, -1);
   return -1;
}

int MPU::write(BYTE b)
{
   unsigned i;

   for (i=0; i<TIMEOUT; i++) {
      cli();
      if (!(inp(usStatPort) & DRR)) {
         outp(usDataPort,b);
         sti();
//         Trace(TRACE_MPUHW_WRITE, b);
         return TRUE;
      }
      sti();
      iodelay(1);
   }

   Trace(TRACE_MPUHW_WRITE, 0xFFFF0000 + b);
   return FALSE;
}

int MPU::command(BYTE b)
{
   Trace(TRACE_MPUHW_CMD, b);
   unsigned i;
//   printf("MPU%i: Sending cmd %x to port %x\n",iPort,(USHORT) b, usStatPort);

   i=1000;
   while (--i) {
      if (inp(usStatPort) & DSR) break;
      inp(usDataPort);
      iodelay(1);
   }
   if (!i)                                      // flushed 1000 bytes?!?!?
      return 4;                                 // Something's gotta be wrong

   for (i=0; i<TIMEOUT; i++) {
      if (!(inp(usStatPort) & DRR)) {      // wait until it's ready
//         printf("MPU: Ready to accept command, i=%i\n",i);
         iodelay(1);                // just to be safe
         outp(usStatPort,b);
         iodelay(1);                // just to be safe
         for (i=0; i<TIMEOUT; i++) {
            if (!(inp(usStatPort) & DSR)) {
//               printf("MPU%i: Cmd ack'd, time=%i",iPort,i);
               i=inp(usDataPort);
//               printf(" and the response is %x\n",i);
               iodelay(1);                // just to be safe
               return i == 0xFE ? 0 : 3;
            }
            iodelay(1);
         }
//         printf("MPU%i: Cmd not ack'd, RC=1\n",iPort);
         if (b == 0xFF) {
//            printf("  Command is reset - ignoring error\n");
            return 0;
         } else
            return 1;
      }
      iodelay(1);
   }

//   printf("MPU%i: Timed out, RC=2\n",iPort);
   return 2;
}

int MPU::init(void)
{
   Trace(TRACE_MPU_INIT, 0);

   if (command(0xFF))
      return FALSE;

   unsigned i = command(0x3F);

   return fQuietInit ? TRUE : (i == 0);
}

int MPU::start(void)
{
   Trace(TRACE_MPU_START, 0);

   return init();
}

void MPU::stop(void)
{
   Trace(TRACE_MPU_STOP, 0);

   silence();
}

void MPU::pause(void)
{
   Trace(TRACE_MPU_PAUSE, 0);

   silence();
}

void MPU::resume(void)
{
   Trace(TRACE_MPU_RESUME, 0);
}

void MPU::setIRQ(IRQ *_pirq)
{
   ASSERT(_pirq);
   pirq = _pirq;
};

void MPU::silence(void)
{
   Trace(TRACE_MPU_SILENCE, 0);

   for (int iChannel=0; iChannel<16; iChannel++) {
      write((BYTE) (0xB0 + iChannel));    // channel mode
      write(121);                        // all controllers off
      write(0);
   }

   for (iChannel=0; iChannel<16; iChannel++) {
      write((BYTE) (0xB0 + iChannel));    // channel mode
      write(123);                        // all notes off
      write(0);
   }
}

// ---------------------

MPUISR::MPUISR(IRQ *_pirq, MPU *_pmpu)
   : ISR(_pirq)
{
   ASSERT(_pmpu);
   pmpu = _pmpu;
}

#pragma off (unreferenced)

int MPUISR::handler(unsigned irq)
{
   int i;

   i = pmpu->read();
   if (i<0) return FALSE;

   Trace(TRACE_MPUHW_ISR, i);
   pfnSendByte(pmpu->handle(),(BYTE) i);
   iodelay(1);

   while ( !(inp(pmpu->port() + 1) & DSR) ) {
      i=pmpu->read();
      if (i<0) return FALSE;

      Trace(TRACE_MPUHW_ISR, i);
      pfnSendByte(pmpu->handle(),(BYTE) i);
      iodelay(1);
   }

   return TRUE;
}

#pragma on (unreferenced)

// ---------------------

int MPUread(USHORT usDataPort)
{
   unsigned i;

   for (i=0; i<TIMEOUT; i++) {
      cli();
      if (!(inp(usDataPort + 1) & DSR)) {
         i=inp(usDataPort);
         sti();
         return i;
      }
      sti();
      iodelay(1);
   }
   return -1;
}

int MPUwrite(USHORT usDataPort, BYTE b)
{
   unsigned i;

   for (i=0; i<TIMEOUT; i++) {
      cli();
      if (!(inp(usDataPort + 1) & DRR)) {
         outp(usDataPort,b);
         sti();
         return TRUE;
      }
      sti();
      iodelay(1);
   }
   return FALSE;
}

int MPUcommand(USHORT usDataPort, BYTE b)
{
   unsigned i;
   const usStatPort = usDataPort + 1;

//   printf("MPU%i: Sending cmd %x to port %x\n",iPort,(USHORT) b, usStatPort);

   i=1000;
   while (--i) {
      if (inp(usStatPort) & DSR) break;
      inp(usDataPort);
      iodelay(1);
   }
   if (!i)                                      // flushed 1000 bytes?!?!?
      return 4;                                 // Something's gotta be wrong

   for (i=0; i<TIMEOUT; i++) {
      if (!(inp(usStatPort) & DRR)) {      // wait until it's ready
//         printf("MPU: Ready to accept command, i=%i\n",i);
         iodelay(1);                // just to be safe
         outp(usStatPort,b);
         iodelay(1);                // just to be safe
         for (i=0; i<TIMEOUT; i++) {
            if (!(inp(usStatPort) & DSR)) {
//               printf("MPU%i: Cmd ack'd, time=%i",iPort,i);
               i=inp(usDataPort);
//               printf(" and the response is %x\n",i);
               iodelay(1);                // just to be safe
               return i == 0xFE ? 0 : 3;
            }
            iodelay(1);
         }
//         printf("MPU%i: Cmd not ack'd, RC=1\n",iPort);
         if (b == 0xFF) {
//            printf("  Command is reset - ignoring error\n");
            return 0;
         } else
            return 1;
      }
      iodelay(1);
   }

//   printf("MPU%i: Timed out, RC=2\n",iPort);
   return 2;
}

int MPUinit(USHORT usDataPort)
{
   int i=MPUcommand(usDataPort, 0xFF);

   if (fQuietInit) {
      MPUcommand(usDataPort, 0x3F);
      return 0;
   }

   return i ? i+4 : MPUcommand(usDataPort, 0x3F);
}
