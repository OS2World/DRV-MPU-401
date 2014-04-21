/* ISR.HPP

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   10-Jun-98  Timur Tabi   Creation
*/

#ifndef ISR_INCLUDE
#define ISR_INCLUDE

#ifndef OS2_INCLUDED
#define INCL_NOPMAPI
#include <os2.h>
#endif

#include "..\midi\midi_idc.h"

extern unsigned int fMCA;

extern PFNMIDI_SENDBYTE pfnSendByte;

class ISR;

class IRQ {
public:
   IRQ(unsigned __irq, int _fShared);
   virtual ~IRQ(void);
   virtual int enable(void);
   virtual void disable(void);
   int isEnabled(void) {return fEnabled; };
   virtual void handler(void);
   unsigned irq(void) { return _irq; };
   void setExclusive(void) { fExclusive = TRUE; };
   unsigned int isExclusive(void) { return fExclusive; };
protected:
   unsigned _irq;
   unsigned int fEnabled;
   unsigned int fShared;      // True if this is a Microchannel machine
   unsigned int fExclusive;   // True if this IRQ has exclusive RM access
   ISR *pisrHead;             // ptr to linked list of ISRs
   void add(ISR *);
   void remove(ISR *);

   friend class ISR;
};

class ISR {
public:
   ISR(IRQ *);
   ~ISR(void);
   virtual int handler(unsigned irq) = 0;
   IRQ *irq(void) { return pirq; };

   ISR *next;
private:
   ISR *prev;
   IRQ *pirq;

   friend class IRQ;
//   friend class TIMER0;
};

#endif

