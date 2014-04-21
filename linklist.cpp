/* LINKLIST.CPP - Linked list class
*/

#include "linklist.hpp"
#include <include.h>

void LINKLIST::init(void)
{
   plHead = NULL;
   plTail = NULL;
}

LINKLIST::~LINKLIST(void)
{
   LINKABLE *pl = plHead;
   LINKABLE *plTemp;

   while (pl) {
      plTemp = pl->plNext;
      if (pl->pll != this)
         int3();
         delete pl;
      pl = plTemp;
   }
}

LINKABLE::LINKABLE(LINKLIST *_pll )
{
   ASSERT(_pll);
   pll = _pll;

   plBefore = pll->plTail;
   pll->plTail = this;

   plNext = NULL;
   if (plBefore)
      plBefore->plNext = this;

   if (!pll->plHead)
      pll->plHead = this;
}

LINKABLE::~LINKABLE(void)
{
   if (this == pll->plHead)
      pll->plHead = plNext;
   if (this == pll->plTail)
      pll->plTail = plBefore;

   if (plBefore)                   // is this the head pointer?
      plBefore->plNext = plNext;   // no, so connect the previous element to the next
   if (plNext)                     // is the is the tail pointer?
      plNext->plBefore = plBefore; // no, so connect the next element to the previous
}

LINKABLE::LINKABLE(void)
{
   pll = NULL;
   plBefore = NULL;
   plNext = NULL;
}

void LINKABLE::MakeFirst(void)
{
   if (plBefore)                   // is this the head pointer?
      plBefore->plNext = plNext;   // no, so connect the previous element to the plNext
   if (plNext)                     // is the is the tail pointer?
      plNext->plBefore = plBefore; // no, so connect the plNext element to the previous
   plNext = pll->plHead;
   pll->plHead = this;
}

void LINKABLE::setList(LINKLIST *_pll)
{
   ASSERT(_pll);
   pll = _pll;

   plBefore = pll->plTail;
   pll->plTail = this;

   plNext = NULL;
   if (plBefore)
      plBefore->plNext = this;

   if (!pll->plHead)
      pll->plHead = this;
}

void LINKABLE::MakeLast(void)
{
   if (plBefore)                   // is this the head pointer?
      plBefore->plNext = plNext;   // no, so connect the previous element to the plNext
   if (plNext)                     // is the is the tail pointer?
      plNext->plBefore = plBefore; // no, so connect the plNext element to the previous
   plBefore = pll->plTail;
   pll->plTail = this;
}

