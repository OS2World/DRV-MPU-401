/* EVENT.HPP
*/

#ifndef EVENT_INCLUDED
#define EVENT_INCLUDED

#ifndef OS2_INCLUDED
#define INCL_NOPMAPI
#include <os2.h>
#endif

#ifndef DDCMD_REG_STREAM         // shdd.h can't handle being included twice
#include <shdd.h>                // for PDDCMDREGISTER
#endif

class EVENT : public LINKABLE {
public:
   void Report(void);         // reports this event
   void SetNext(void);        // sets peventNext to the next event

   friend void DeleteEvents(STREAM *); // needs STREAM *pstream
   friend EVENT *FindEvent(HEVENT);    // needs HEVENT he
   friend void STREAM::ProcessEvents(void);  // needs ULONG ulNextTime

/*
   void *operator new(size_t);         // used to create a new event
   void *operator new(size_t, void *p) // used to update an exist event
      { return p; }
   void operator delete(void *);
*/
   EVENT(STREAM *, HEVENT, PCONTROL_PARM);
   ~EVENT(void);
private:
   HEVENT hevent;                 // the event handle
   STREAM *pstream;           // the stream for this event
   ULONG ulRepeatTime;        // for recurring events
   ULONG ulNextTime;          // the time this event should occur
   unsigned int fRecurring;            // true if this is a recurring event
   SHD_REPORTEVENT shdre;
};

extern EVENT *peventNext;        // the pointer to the next event
extern unsigned int fRecurringSupported;  // TRUE if recurring events are supported
extern LINKLIST llEvents;

EVENT *FindEvent(HEVENT);
// Given an event handle (HEVENT), return the pointer to the
// corresponding event object

void DeleteEvents(STREAM *);
// Delete all events that belong to this stream.  Used by the stream class
// when a stream is deleted.

#endif
