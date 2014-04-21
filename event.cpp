/* EVENT.CPP - Stream event handler

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
*/

#define INCL_NOPMAPI
#define INCL_DOSERRORS           // for ERROR_INVALID_FUNCTION
#include <os2.h>
#include <stddef.h>
#include <os2me.h>
#ifndef DDCMD_REG_STREAM         // shdd.h can't handle being included twice
#include <shdd.h>
#endif

#include <include.h>
#include "..\printf\printf.h"

#include "stream.hpp"
#include "event.hpp"

LINKLIST llEvents;

unsigned int fRecurringSupported = FALSE; // TRUE if recurring events are supported
EVENT *peventNext = NULL;          // the pointer to the next event

void EVENT::SetNext(void)
{
   ULONG ulTimeToBeat = -1;     // -1 equals 0xFFFFFFFF, the maximum time
   peventNext = NULL;

   EVENT *pevent = (EVENT *) llEvents.Head();

   while (pevent) {
      if ( pevent->ulNextTime < ulTimeToBeat ) {
         peventNext = pevent;
         ulTimeToBeat = pevent->ulNextTime;
      }
      pevent = (EVENT *) pevent->plNext;
   }
}

void EVENT::Report(void)
{
// Report the event.  If an error occurs, then something strange
// happened.
   switch (pstream->pfnSHD(&shdre)) {
      case ERROR_INVALID_EVENT:     // bad event? someone else must have
         delete this;               //  deleted it.  So we'll just remove it
         return;                    //  from our list and forget it
      case ERROR_INVALID_STREAM:    // bad stream?
         delete pstream;
         return;
   }

   if (fRecurring) {
      ulNextTime += ulRepeatTime;        // calculate new value for peventNext
      shdre.ulStreamTime = ulNextTime;
      SetNext();
   } else
      delete this;
}

EVENT::EVENT(STREAM *ps, HEVENT hevent, PCONTROL_PARM pcp)
   : LINKABLE(&llEvents)

{
   pstream = ps;
   hevent = hevent;

// If recurring events are support, check to see if this is a recurring event.
   if (fRecurringSupported)
      fRecurring = (int) (pcp->evcb.ulFlags & EVENT_RECURRING);
   else
      fRecurring = FALSE;

   if (fRecurring) {
      ulRepeatTime = pcp->ulTime;
      ulNextTime = pstream->ulCurrentTime+ulRepeatTime;
   } else
      ulNextTime = pcp->ulTime;

   shdre.ulFunction = SHD_REPORT_EVENT;
   shdre.hStream = pstream->hstream;
   shdre.hEvent = hevent;
   shdre.ulStreamTime = ulNextTime;

   if (!peventNext) {      // Is there a next event?
      peventNext = this;   // no, make this event the next one
      return;
   }

// There already is a next event.  So clear interrupts to make sure
// that peventNext isn't modified during an interrupt.  Then check to
// see if this event will occur before the other event.  If so, make
// this event the next one

   cli();
   if (ulNextTime < peventNext->ulNextTime)
      peventNext = this;
   sti();
}

EVENT::~EVENT(void)
{
// By setting ulNextTime to -1, it will never pass the condition
// "pevent->ulNextTime < ulTimeToBeat" in SetNext().

   if (peventNext == this) {  // are we deleting the next event?
      ulNextTime = -1;        // Yes. Make sure it's not chosen again
      SetNext();              // Now find another one
   }
}

EVENT *FindEvent(HEVENT hevent)
{
   EVENT *pevent = (EVENT *) llEvents.Head();

   while (pevent) {
      if ( pevent->hevent == hevent )
         return pevent;
      pevent = (EVENT *) pevent->plNext;
   }

   return NULL;
}

void DeleteEvents(STREAM *pstream)
{
   EVENT *pevent = (EVENT *) llEvents.Head();

   while (pevent) {
      if ( pevent->pstream == pstream )
         delete pevent;
      pevent = (EVENT *) pevent->plNext;
   }
}

