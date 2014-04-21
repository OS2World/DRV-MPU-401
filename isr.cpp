/* ISR.CPP - Handles interrupts from MPU-401 hardware

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
   18-Apr-96  Timur Tabi   updated npfnISR for Watcom 10.6 compatibility
   20-Nov-96  Timur Tabi   removed #include "..\printf\printf.h"
*/

#define INCL_NOPMAPI
#include <os2.h>

#include <devhelp.h>
#include <include.h>

#include "iodelay.h"
#include "isr.hpp"

// #include "..\midi\midi_idc.h"
// #include "mpu401.hpp"

// True if this is a Microchannel machine
unsigned int fMCA = FALSE;

IRQ *apirq[16] = {NULL};

IRQ::IRQ(unsigned __irq, int _fShared)
{
   ASSERT(__irq < 16);
   ASSERT(apirq[__irq] == NULL);

   _irq = __irq;
   fShared = _fShared;
   pisrHead = NULL;
   fExclusive = FALSE;

   apirq[__irq] = this;
}

IRQ::~IRQ(void)
{
   if (fEnabled)
      disable();

   apirq[_irq] = NULL;
}

void IRQ::handler(void) {
   ISR *pisr = pisrHead;

   while (pisr) {
      if (pisr->handler(_irq)) {
         cli();            // prevent nesting of interrupts
         DevHelp_EOI(_irq);
         clc();
         return;
      } else
      pisr = pisr->next;
   }

   if (fShared)
      stc();
   else
      clc();
}

void __far MPU401_ISR3(void)
{
   ASSERT(apirq[3]);
   if (apirq[3]) apirq[3]->handler();
}

void __far MPU401_ISR4(void)
{
   ASSERT(apirq[4]);
   if (apirq[4]) apirq[4]->handler();
}

void __far MPU401_ISR5(void)
{
   ASSERT(apirq[5]);
   if (apirq[5]) apirq[5]->handler();
}

void __far MPU401_ISR7(void)
{
   ASSERT(apirq[7]);
   if (apirq[7]) apirq[7]->handler();
}

void __far MPU401_ISR9(void)
{
   ASSERT(apirq[9]);
   if (apirq[9]) apirq[9]->handler();
}

void __far MPU401_ISR10(void)
{
   ASSERT(apirq[10]);
   if (apirq[10]) apirq[10]->handler();
}

void __far MPU401_ISR11(void)
{
   ASSERT(apirq[11]);
   if (apirq[11]) apirq[11]->handler();
}

void __far MPU401_ISR12(void)
{
   ASSERT(apirq[12]);
   if (apirq[12]) apirq[12]->handler();
}

void __far MPU401_ISR13(void)
{
   ASSERT(apirq[13]);
   if (apirq[13]) apirq[13]->handler();
}

void __far MPU401_ISR14(void)
{
   ASSERT(apirq[14]);
   if (apirq[14]) apirq[14]->handler();
}

void __far MPU401_ISR15(void)
{
   ASSERT(apirq[15]);
   if (apirq[15]) apirq[15]->handler();
}

NPFN npfnISR[]={
   (NPFN) NULL,
   (NPFN) NULL,
   (NPFN) NULL,
   (NPFN) MPU401_ISR3,
   (NPFN) MPU401_ISR4,
   (NPFN) MPU401_ISR5,
   (NPFN) NULL,
   (NPFN) MPU401_ISR7,
   (NPFN) NULL,
   (NPFN) MPU401_ISR9,
   (NPFN) MPU401_ISR10,
   (NPFN) MPU401_ISR11,
   (NPFN) MPU401_ISR12,
   (NPFN) MPU401_ISR13,
   (NPFN) MPU401_ISR14,
   (NPFN) MPU401_ISR15,
};

int IRQ::enable(void)
{
   if (fEnabled)
      return TRUE;

   if (!npfnISR[_irq])
      return FALSE;

   fEnabled = DevHelp_SetIRQ(npfnISR[_irq], _irq, fShared ? 1 : 0) ? FALSE : TRUE;

   return fEnabled;
}

void IRQ::disable(void)
{
   if (fEnabled) {
      DevHelp_UnSetIRQ(_irq);
      fEnabled = FALSE;
   }
}

void IRQ::add(ISR *pisr)
{
   ASSERT(pisr);

   pisr->next = pisrHead;
   pisr->prev = NULL;
   pisrHead = pisr;
}

void IRQ::remove(ISR *pisr)
{
   ASSERT(pisr);

   if (pisrHead == pisr) {
      pisrHead = pisr->next;
      return;
   }

   if (pisr->prev)
      pisr->prev->next = pisr->next;

   if (pisr->next)
      pisr->next->prev = pisr->prev;

   pisr->next = NULL;
   pisr->prev = NULL;
}

ISR::ISR(IRQ *_pirq)
{
   ASSERT(_pirq);

   pirq = _pirq;
   pirq->add(this);
}

ISR::~ISR(void)
{
   pirq->remove(this);
}
