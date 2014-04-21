/* LINKLIST.HPP - Linked list class
*/

#ifndef LINKLIST_INCLUDED
#define LINKLIST_INCLUDED

/* Implements FIFO queue.  New members are inserted at the "Tail" of the queue.
     LINKLIST::plHead points to front of queue (oldest member, next one to pop()).
     LINKLIST::plTail points to back of queue (newest member, last one to pop()).
     LINKABLE::plBefore points toward the front (head) of the queue.
     LINKABLE::plNext points toward the back (tail) of the queue.
   ###BUG Not clear who sets up the LINKABLE::plNext field, might not be getting set.
*/

class LINKABLE;
class LINKLIST;

// This is an element of the list.  Used as the base
// class for things we want to keep in lists (like
// Streams, Events, AudioHW, etc. etc. etc.
// ptr to the linked list that this object is on

class LINKABLE {
public:
   LINKABLE *plNext;       // Follows me in the queue.
   LINKABLE *plBefore;     // Before me in the queue.

   void MakeFirst(void);
   void MakeLast(void);

   LINKABLE( LINKLIST *pll);
   LINKABLE(void);
   virtual ~LINKABLE(void);
   void setList(LINKLIST *pll);
private:
   LINKLIST *pll;
   friend class LINKLIST;
};


// List head, points to head & tail of list.  "Final"
// class, nothing derived from this.  However, global
// vbls like "StreamList", "AudioHWList" are of this type.

class LINKLIST {
public:
   void init(void);
   ~LINKLIST(void);

   LINKABLE* Head(void) { return plHead; };
private:
   LINKABLE *plHead;
   LINKABLE *plTail;
   friend class LINKABLE;
};

#endif
