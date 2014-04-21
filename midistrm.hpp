/* MIDISTRM.HPP
*/

#include "stream.hpp"

class MIDISTREAM : public STREAM {
public:
   MIDISTREAM(ULONG _ulSysFileNume, MPU *_pmpu);
private:
// Members that deal with timing
   ULONG ulPerClock;             // microseconds per clock
   ULONG ulTempo;                // 1/10 beats per minute (beats per 10 minutes)
   USHORT usCPQN;                // clocks per beats
   void CalcDelay(void);         // calculates ulPerClock

// Members that deal with processing the stream data
   unsigned uiResidue;           // microsecond residue for ulCurrentTime
   unsigned uState;              // the state identifier
   USHORT usCompValue;           // the long time compression value
   long lWait;                   // microseconds to wait until it's time to process the next message
   unsigned int fRunningStat;             // TRUE if this was running status

   void Deal(BYTE);              // processes the next byte of the current buffer
   void Process(void);           // called every timer interrupt

   virtual int Start(void);
   virtual void Stop(void);
   virtual void Pause(void);
   virtual void Resume(void);

   friend void __far __loadds MPU401_Timer0Hook(void);
   friend void __far __loadds __saveregs MPU401_SysTimerHook(void);
};

