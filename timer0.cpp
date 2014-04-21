/* TIMER0.CPP

*/

#define INCL_NOPMAPI
#include <os2.h>

#include <devhelp.h>
#include <include.h>

#include "iodelay.h"
#include "timer0.hpp"
#include "trace.h"

extern IRQ *apirq[];

PTMRFN pfnTimer = NULL;
ULONG ulFakeTimer0;
ULONG __far *pulTimer = &ulFakeTimer0;

TIMER0::TIMER0(void)
   : IRQ(0, FALSE)
{
}

int TIMER0::enable(void)
{
   if (fEnabled)
      return TRUE;

   if (!pfnTimer)
      return FALSE;

   if (pfnTimer(TMR0_REG, (ULONG) callback, 20)) {
      HALT;
      return FALSE;
   }

   fEnabled = TRUE;
   Trace(TRACE_TIMER0_ENABLE, *pulTimer);
   return TRUE;
}

void TIMER0::disable(void)
{
   pfnTimer(TMR0_DEREG, (ULONG) callback, 0);
   Trace(TRACE_TIMER0_DISABLE, *pulTimer);
}

void __far __loadds TIMER0::callback(void)
{
   ASSERT(apirq[0]);
   apirq[0]->handler();
}

void TIMER0::handler(void) {
   ISR *pisr = pisrHead;

   while (pisr) {
      if (pisr->handler(_irq))
         return;
      pisr = pisr->next;
   }
}


