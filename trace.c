/* TRACE.C
*/

#ifdef DEBUG

#define INCL_NOPMAPI
#include <os2.h>
#include <string.h>

#include "trace.h"
#include "end.h"
#include "..\wpddlib\strategy.h"
#include "..\printf\printf.h"

#include <devhelp.h>
#include <include.h>

#define QUEUE_SIZE   64

TRACE_EVENT ate[QUEUE_SIZE];
unsigned uTraceHead = 0;
unsigned uTraceTail = 0;

unsigned int fTraceEnabled = FALSE;
unsigned int fTraceWaiting = FALSE;

STATUS status;

int TraceRead(TRACE_EVENT __far *pte)
{
   if (!fTraceEnabled)
      return FALSE;

   if (uTraceHead == uTraceTail)
      return FALSE;

   cli2();
   pte->ulEvent = ate[uTraceHead].ulEvent;
   pte->ulParameter = ate[uTraceHead].ulParameter;
   uTraceHead = (uTraceHead + 1) % QUEUE_SIZE;
   sti2();

   return TRUE;
}

void Trace(unsigned long ulEvent, unsigned long ulParameter)
{
   USHORT usAwakeCount;

   printf("TRACE %lu/%lu %lx/%lx\n", ulEvent, ulParameter, ulEvent, ulParameter);
   if (fTraceEnabled) {
      cli2();
      ate[uTraceTail].ulEvent = ulEvent;
      ate[uTraceTail].ulParameter = ulParameter;
      uTraceTail = (uTraceTail + 1) % QUEUE_SIZE;
      sti2();

      if (fTraceWaiting) {
         DevHelp_ProcRun((ULONG) (void __far *) &fTraceEnabled, &usAwakeCount);
         printf("TRACE unblocked MPUTRACE_StrategyRead, count=%u\n", usAwakeCount);
      }
   }
}

SEL selTrace;

void MPUTRACE_StrategyInit(PREQPACKET prp)
{
   USHORT rc;

   rc = DevHelp_AllocGDTSelector(&selTrace, 1);
   if (rc)
      int3();

   prp->usStatus = RPDONE;
   prp->s.init_out.usCodeEnd = (USHORT) &end_of_text;
   prp->s.init_out.usDataEnd = (USHORT) &end_of_heap;
}

void MPUTRACE_StrategyOpen(PREQPACKET prp)
{
   printf("MPUTRACE_StrategyOpen\n");

   fTraceEnabled = TRUE;
}

void MPUTRACE_StrategyClose(PREQPACKET prp)
{
   printf("MPUTRACE_StrategyClose\n");

   fTraceEnabled = FALSE;

   if (fTraceWaiting) {
      USHORT usAwakeCount;
      DevHelp_ProcRun((ULONG) (void __far *) &fTraceEnabled, &usAwakeCount);
      fTraceWaiting = FALSE;
   }
}

void MPUTRACE_StrategyRead(PREQPACKET prp)
{
   TRACE_EVENT __far *pte;
   USHORT rc;

   printf("MPUTRACE_StrategyRead\n");

   rc = DevHelp_PhysToGDTSelector(prp->s.io.ulAddress, prp->s.io.usCount, selTrace);
   if (rc)
      int3();

   pte = MAKEP(selTrace, 0);

   while (!TraceRead(pte)) {
      printf("MPUTRACE_StrategyRead blocking\n");
      cli();                                          // yes, then block
      fTraceWaiting = TRUE;
      DevHelp_ProcBlock((ULONG) (void __far *) &fTraceEnabled, -1, 1);
      fTraceWaiting = FALSE;
      printf("MPUTRACE_StrategyRead released\n");
   }
}

void MPUTRACE_IoctlGetStatus(PREQPACKET prp)
{
   _fmemcpy(prp->s.ioctl.pvData, &status, min(sizeof(STATUS), prp->s.ioctl.usDLength));
}

void MPUTRACE_StrategyIoctl(PREQPACKET prp)
{
   if (prp->s.ioctl.bCategory != 0x80) {
      prp->usStatus |= RPERR | RPBADCMD;
      return;
   }

   switch (prp->s.ioctl.bCode) {
      case 0:
         MPUTRACE_IoctlGetStatus(prp);
         break;
      default:
         prp->usStatus |= RPERR | RPBADCMD;
         return;
   }
}

#pragma aux MPUTRACE_StrategyHandler parm [es bx];
void __far MPUTRACE_StrategyHandler(PREQPACKET prp)
{
   prp->usStatus = RPDONE;

   switch (prp->bCommand) {
      case STRATEGY_INIT:
         MPUTRACE_StrategyInit(prp);
         break;
      case STRATEGY_OPEN:
         MPUTRACE_StrategyOpen(prp);
         break;
      case STRATEGY_CLOSE:
         MPUTRACE_StrategyClose(prp);
         break;
      case STRATEGY_GENIOCTL:
         MPUTRACE_StrategyIoctl(prp);
         break;
      case STRATEGY_READ:
         MPUTRACE_StrategyRead(prp);
         break;
      default:
         prp->usStatus = RPDONE | RPERR | RPGENFAIL;
   }
}

#endif
