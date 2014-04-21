/* MALLOC.C - simple heap for PDD's

   MODIFICATION HISTORY
   VERSION    PROGRAMMER   COMMENT
   13-Apr-98  Timur Tabi   Creation
*/

#include <string.h>
#include <include.h>
#include "end.h"
// #include "..\printf\printf.h"

// If SIGNATURE is defined, then each MEMBLOCK will include a signature
// This is useful for debugging.  It verifies that a free() request points to
//  a valid block.  The signature is a 32-bit value that should be somewhat
//  unique and identifiable
#ifdef DEBUG
#define  SIGNATURE      0x12345678
#endif

#pragma pack(1)

typedef struct _MEMBLOCK {
   unsigned uSize;
   struct _MEMBLOCK *pmbNext;
#ifdef SIGNATURE
   unsigned long ulSignature;
#endif
   char achBlock[1];
} MEMBLOCK, __near *PMEMBLOCK;

#pragma pack()

// HDR_SIZE must always be a multiple of 4, to ensure proper alignment
// and to make sure all the pointer math works
#define HDR_SIZE ( sizeof(MEMBLOCK) - 1 )

PMEMBLOCK pmbUsed = NULL;
extern PMEMBLOCK pmbFree;

unsigned _msize(void __near *pvBlock)
{
   PMEMBLOCK pmb;

   if (!pvBlock)
      return 0;

   pmb = (PMEMBLOCK) ((char __near *) pvBlock - HDR_SIZE);

   return pmb->uSize;
}

/* malloc()
This function searches the list of free blocks until it finds one big enough
to hold the amount of memory requested, which is passed in uSize.  In order
for a free block to be "big enough", it not only has to hold the requested
memory, but it must also be able to hold another free block.  This is because
the remainder of the free block that is not converted to a used block
must be converted into another free block.  In other words:

    _______________________free block_______________________________________
   |  free    |                                                             |
   |  block   |       space available                                       |
   |  header  |          (pmbFree->uSize bytes long)                        |
   |(HDR_SIZE |                                                             |
   |__bytes)__|_____________________________________________________________|

   <--- h ---> <------------------------- l -------------------------------->

Must become:

    _______________________used block_______________________________________
   |  used    |                                     |  free    |            |
   |  block   |  space allocated                    |  block   | space      |
   |  header  |     (pmbUsed->uSize bytes long)     |  header  | available  |
   |(HDR_SIZE |                                     |(HDR_SIZE |            |
   |__bytes)__|_____________________________________|__bytes)__|____________|

   <--- h ---> <-------------- n ------------------><--- h ---> <--- m ----->

To keep the programming simple, m must be at least 4 bytes long.  This
insures the make_new_free() can always return a non-zero new free block.

This means that in order for a free block to be chosen, this equation must
hold: l >= n + h + 4.  The local variable uFitSize is equal to n + h + 4.
*/

/* NOTE: if the size of the block to allocate is equal to the size of the
   free block, we should remove the free block from the list, rather than
   just set its size to zero
*/
PMEMBLOCK make_new_free(PMEMBLOCK pmbOldFree, unsigned uSize) {
   PMEMBLOCK pmbNewFree;

   pmbNewFree = (PMEMBLOCK) ((char __near *) pmbOldFree + uSize + HDR_SIZE);
   pmbNewFree->uSize = pmbOldFree->uSize - uSize - HDR_SIZE;
   pmbNewFree->pmbNext = pmbOldFree->pmbNext;

   return pmbNewFree;
}

void __near *malloc(unsigned uSize)
{
   PMEMBLOCK pmb, pmbPrev;
   unsigned uFitSize;

   ASSERT(check_heap());

   if (!uSize)
      return 0;

   uSize = (uSize + 3) & -4;        // round up to nearest DWORD
   uFitSize = uSize + HDR_SIZE + 4; // This is how big a free block needs to be

// Now we start checking each free block.  Note that during the first pass of
// this loop, pmbPrev is undefined.  However, if the first block we find is
// big enough, then we need to update pmbFree instead.

   for (pmb = pmbFree; pmb; pmb = pmb->pmbNext) {
      if (pmb->uSize >= uFitSize) {               // Did we find a block big enough?
         if (pmb == pmbFree)                      // Yes. Is it the first one?
            pmbFree = make_new_free(pmb, uSize);  // Yes. So we have a new pmbFree
         else
            pmbPrev->pmbNext = make_new_free(pmb, uSize);

         pmb->pmbNext = pmbUsed;       // This new block becomes the first used block
         pmb->uSize = uSize;
#ifdef SIGNATURE
         pmb->ulSignature = SIGNATURE;
#endif
         pmbUsed = pmb;

         uMemFree -= (uSize + HDR_SIZE);
         return (void __near *) pmb->achBlock;
      }
      pmbPrev = pmb;
   }

   return 0;
}

/* void compact(void)
This function compacts the free blocks together.  This function is a
companion to free(), and thus the algorithm is tailored to how free()
works.  Change the algorithm in free(), and you'll have to change this
function too.

When free() frees a block, it sets the head of the free list, pmbFree, to
point to it.  Then the newly freed block points to whatever the old pmbFree
pointed to.  In other words, each new free block is added to the head of
the free list.

If compact() is always called after a block is freed, then it can be
guaranteed that the free list is always compacted (i.e. you won't find
two adjacent free blocks anywhere in the heap) _before_ each call to free().
Therefore, the newly freed block can be located in only one of the
following positions:
1. Not adjacent to any other free blocks (compacting not necessary)
2. Physically before another free block
3. Physically after another free block
4. Between two free blocks (i.e. the block occupies the entire space
   between two free blocks)

Since the newly freed block is the first in the free list, compact()
starts with the second block in the list (i.e. pmbFree->pmbNext).
Each free block is then compared with the newly freed block for
adjacency.  If a given block is located before the new block, then it
can't possibly be also located after, and vice versa.  Hence, the
"else if" statement in the middle of the loop.

Also, the newly freed block can only be adjacent to at most two
other blocks.  Therefore, the operation of combining two adjacent
free blocks can only happen at most twice.  Hence, the boolean variable
fFreedOne.  After two blocks are combined, if fFreedOne is TRUE, the
routine exits.  fFreedOne is initially false.  The 1st time two blocks are
combined, it is set to TRUE, and the routine continues.  The 2nd time
this happens, fFreedOne is already TRUE, so the routine knows that it
is done.

Helper macro after() takes a PMEMBLOCK (call it pmb) as a parameter,
and calculates where an adjacent free block would exist if it were
physically located after pmb.

Helper function remove() removes an element from the free list.  If
the block to remove is the head of the free list, it simply
*/

#define after(pmb) ((PMEMBLOCK) ((char __near *) pmb + pmb->uSize + HDR_SIZE))

void remove(PMEMBLOCK pmb)
{
   PMEMBLOCK pmbPrev;

   if (pmb == pmbFree) {
      pmbFree = pmbFree->pmbNext;
      return;
   }

   for (pmbPrev = pmbFree; pmbPrev; pmbPrev = pmbPrev->pmbNext)
      if (pmbPrev->pmbNext == pmb) {
         pmbPrev->pmbNext = pmb->pmbNext;
         return;
      }
}

void compact(void)
{
   PMEMBLOCK pmb;
   unsigned int fFreedOne = FALSE;

   for (pmb = pmbFree->pmbNext; pmb; pmb = pmb->pmbNext)
      if (after(pmb)  == pmbFree) {
         // There is a free block located AFTER our block
         pmb->uSize += HDR_SIZE + pmbFree->uSize;
         remove(pmbFree);
         if (fFreedOne) return;
         fFreedOne = TRUE;
      } else if (after(pmbFree) == pmb) {
         // There is a free block located BEFORE our block
         pmbFree->uSize += HDR_SIZE + pmb->uSize;
         remove(pmb);
         if (fFreedOne) return;
         fFreedOne = TRUE;
      }
}

void free(void __near *pvBlock)
{
   PMEMBLOCK pmb, pmbPrev, pmbBlock;

   ASSERT(pvBlock);
   if (!pvBlock) return;     // support freeing of NULL in non-debug mode

   ASSERT(validate(pvBlock));
   ASSERT(check_heap());

   pmbBlock = (PMEMBLOCK) ((char __near *) pvBlock - HDR_SIZE);
   ASSERT(pmbBlock->ulSignature == SIGNATURE);
   uMemFree += pmbBlock->uSize + HDR_SIZE;

   pmbPrev = NULL;
   for (pmb = pmbUsed; pmb; pmb = pmb->pmbNext) {
      if (pmb == pmbBlock) {
         if (pmb == pmbUsed)              // Are we deleting the first block?
            pmbUsed = pmbUsed->pmbNext;   // Yes, so adjust pmbUsed
         if (pmbPrev)
            pmbPrev->pmbNext = pmb->pmbNext;
#ifdef SIGNATURE
         pmbBlock->ulSignature = 0;
#endif
         pmbBlock->pmbNext = pmbFree;     // this block is now free, so point it to 1st free block
         pmbFree = pmbBlock;              // and then it becomes the first free block
         compact();
      }
      pmbPrev = pmb;
   }
}

unsigned _memfree(void)
{
#ifdef DEBUG
   PMEMBLOCK pmb = pmbFree;
   unsigned uTempFree = 0;

   while (pmb) {
      uTempFree += pmb->uSize;
      pmb = pmb->pmbNext;
   }

   ASSERT(uTempFree == uMemFree);
#endif

   return uMemFree;
}

void __near *realloc(void __near *pvBlock, unsigned usLength)
{
   void __near *pv;

   if (!pvBlock)                 // if ptr is null, alloc block
      return malloc(usLength);

   if (!usLength) {              // if length is 0, free block
      free(pvBlock);
      return 0;
   }

   pv=malloc(usLength);          // attempt to allocate the new block
   if (!pv)                      // can't do it?
      return 0;                  // just fail.  Version 2 will try harder

   memcpy(pv, pvBlock, min(_msize(pvBlock), usLength));
   free(pvBlock);
   return pv;
}

int validate(void __near *p)
{
   PMEMBLOCK pmb = pmbUsed;

   while (pmb)
   {
      if (p == pmb->achBlock)
      {
#ifdef SIGNATURE
         if (pmb->ulSignature != SIGNATURE)
            return FALSE;
#endif
         return TRUE;
      }
      pmb = pmb->pmbNext;
   }

   return FALSE;
}


/* int quick_validate(void __near *p)
This function does a quick verification of the pointer passed
to it.  It's most useful if signatures have been enabled.
*/

int quick_validate(void __near *p)
{
#ifdef SIGNATURE
   PMEMBLOCK pmb;
#endif

   if ((p < &mbFirstFree) || (p > &end_of_heap))
      return FALSE;

#ifdef SIGNATURE
   pmb = (PMEMBLOCK) (((char __near *) p) - HDR_SIZE);
   if (pmb->ulSignature != SIGNATURE)
      return FALSE;
#endif

   return TRUE;
}

/* int check_heap(void)
This function checks to make sure the heap is not corrupt.  It scans
every free and used block for overlaps.

This function starts by looping through every used block.  For each
used block, it compares it with every other used block as well as every
free block.  This means that if n is the number of used blocks, and
m is the number of free blocks, then the execution time is O(n*n*m).
*/

int check_heap(void)
{
   PMEMBLOCK pmb, pmb2;
   char *p, *p2;

   pmb = pmbUsed;
   while (pmb) {                 // loop through all used blocks
      p = (char *) pmb;
      pmb2 = pmbUsed;
      while (pmb2) {             // loop again through all used blocks
         if (pmb != pmb2) {      // but skip the block we're checking
            p2 = (char *) pmb2;
            if (pmb < pmb2) {
               if (p + HDR_SIZE + pmb->uSize > p2)
                  return FALSE;
            } else {
               if (p2 + HDR_SIZE + pmb2->uSize > p)
                  return FALSE;
            }
         }
         pmb2 = pmb2->pmbNext;
      }

      pmb2 = pmbFree;
      while (pmb2) {             // now loop through all the free blocks
         p2 = (char *) pmb2;
         if (pmb < pmb2) {
            if (p + HDR_SIZE + pmb->uSize > p2)
               return FALSE;
         } else {
            if (p2 + HDR_SIZE + pmb2->uSize > p)
               return FALSE;
         }
         pmb2 = pmb2->pmbNext;
      }
      pmb = pmb->pmbNext;
   }

   return TRUE;
}
