/* TIMER0.HPP
*/

#ifndef TIMER0_INCLUDE
#define TIMER0_INCLUDE

#include "isr.hpp"
#include "..\timer0\tmr0_idc.h"

class TIMER0 : public IRQ {
public:
   TIMER0(void);
   virtual int enable(void);
   virtual void disable(void);
   virtual void handler(void);
private:
   static void __far __loadds callback(void);
};

extern PTMRFN pfnTimer;
extern ULONG __far *pulTimer;

#endif
