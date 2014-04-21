/* STREAM.HPP
*/

#ifndef STREAM_INCLUDED
#define STREAM_INCLUDED

#ifndef OS2_INCLUDED
#define INCL_NOPMAPI
#include <os2.h>
#endif

#include <stddef.h>              // for size_t

#ifndef OS2ME_INCLUDED
#include <os2me.h>
#endif

#ifndef _SSM_H_
#include <ssm.h>
#endif

#ifndef DDCMD_REG_STREAM         // shdd.h can't handle being included twice
#include <shdd.h>                // for PDDCMDREGISTER
#endif

#include "linklist.hpp"
#include "mpu401.hpp"

#define MAX_BUFFERS  4           // must be a power of 2 and greater than 2

typedef char __far *PSTREAMBUF;
typedef ULONG (__far __cdecl *PFN_SHD) (void __far *);

// extern "C" void InitStream(void);
extern "C" ULONG __far __loadds __cdecl DDCMD_EntryPoint(PDDCMDCOMMON pCommon);

class STREAM : public LINKABLE {
public:
   unsigned int fActive;                  // true if the stream is running
   unsigned int fPaused;                  // true if the stream is paused
   unsigned int fIncrementCounter;        // true if the current time should be incremented on every tick
   AUDIOHW *phw;

   ULONG ulCurrentTime;

   BYTE bVolume;                 // Per-stream volume, 128 is max (100%)

   ULONG Control(PDDCMDCONTROL);
   ULONG Write(PSTREAMBUF, unsigned);
   void Register(PDDCMDREGISTER);
   virtual void DeRegister(void);
   STREAM(ULONG _ulSysFileNume);
   virtual ~STREAM(void);
protected:
   PSTREAMBUF apbuf[MAX_BUFFERS];   // pointer to each buffer
   unsigned auBufLen[MAX_BUFFERS];  // lengths of each buffer
   unsigned uBufIdx;             // index into the current buffer
   unsigned uHead;               // buffer queue head
   unsigned uTail;               // buffer queue tail

   void ReturnBuffer(unsigned);  // returns one buffer
   void ReturnBuffers(void);     // returns all buffers to the stream handler
   void ProcessEvents(void);

   virtual int Start(void) = 0;     // These routines are intended to allow the
   virtual void Stop(void) = 0;     //  the derived classes to take action
   virtual void Pause(void) = 0;    //  whenever the stream changes state
   virtual void Resume(void) = 0;
private:
   ULONG ulSysFileNum;
   HSTREAM hstream;
   PFN_SHD pfnSHD;

   friend STREAM *FindStream(HSTREAM);
   friend STREAM *FindFile(ULONG);
   friend void InitStream(void);
   friend ULONG __far __loadds __cdecl DDCMD_EntryPoint(PDDCMDCOMMON pCommon);
   friend class EVENT;
};

extern BYTE bMasterVolume;          // The master volume, 128 is maximum (100%)
extern unsigned uNumStreams;
extern LINKLIST llStreams;

STREAM *FindStream(HSTREAM);
// Returns the pointer to the STREAM object with the given stream handle

STREAM *FindFile(ULONG);
// Returns the pointer to the STREAM object with the given file handle

#endif
