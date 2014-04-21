/* MPU401.H

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
   16-Nov-95  Timur Tabi   Added support for Quiet Initializations
*/

#ifndef MPU401_INCLUDED
#define MPU401_INCLUDED

#include <include.h>

#include "linklist.hpp"
#include "isr.hpp"
#define DSR 0x80
#define DRR 0x40

extern LINKLIST llMPU;

class AUDIOHW : public LINKABLE
{
public:
   AUDIOHW();
   virtual int start(void) = 0;
   virtual void stop(void) = 0;
   virtual void pause(void) = 0;
   virtual void resume(void)= 0;
   virtual void silence(void) = 0;
};

class MPU : public AUDIOHW
{
public:
   MPU(USHORT);
   virtual int start(void);
   virtual void stop(void);
   virtual void pause(void);
   virtual void resume(void);
   virtual void silence(void);

   int init(void);
   int read(void);
   int write(BYTE);
   int command(BYTE);

   USHORT port(void) { return usDataPort; };
   IRQ *irq(void) { return pirq; };
   ULONG handle(void) { return ulHandle; };

   void setHandle(ULONG _ulHandle) { ulHandle = _ulHandle; };
   void setIRQ(IRQ *_pirq);

   void setExclusive(void) { fExclusive = TRUE; };
   unsigned int isExclusive(void) { return fExclusive; };
private:
   ULONG ulHandle;
   USHORT usDataPort;
   USHORT usStatPort;
   unsigned int fExclusive;   // True if the I/O port has exclusive RM access
   IRQ *pirq;
};

class MPUISR : public ISR {
public:
   virtual int handler(unsigned irq);
   MPUISR(IRQ *, MPU *);
private:
   MPU *pmpu;
};

extern unsigned int fQuietInit;
// If TRUE, then ignore errors from hardware initialization

extern unsigned int fSMP;
// True if this is an SMP machine

int MPUread(USHORT);
// reads data port.  If error, returns -1, otherwise data read is in LSB and MSB=0

int MPUwrite(USHORT, BYTE);
// writes byte to data port.  Returns 0 if failure, 1 if success

int MPUcommand(USHORT, BYTE);
// sends byte to an I/O address.  Returns non-zero if error

int MPUinit(USHORT);
// Resets the MPU-401 and switches it to UART mode

#endif
