/* MONO.C - MDA write routines

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   25-Nov-96  Timur Tabi   Creation
*/

#include <include.h>
#include <devhelp.h>
#include <string.h>
#include "iodelay.h"

USHORT __far *pusMonoBuf3 = NULL;
USHORT __far *pusMonoBuf0 = NULL;
USHORT __far *pusMonoBuf = NULL;

unsigned int fRing3 = TRUE;
unsigned int fIsMono = FALSE;

#ifndef MAKEUSHORT
#define MAKEUSHORT(l, h) (((USHORT)(l)) | ((USHORT)(h)) << 8)
#endif


#ifndef MAKEULONG
#define MAKEULONG(l, h)  ((ULONG)(((USHORT)(l)) | ((ULONG)((USHORT)(h))) << 16))
#endif

#define MONO_PORT    0x3b4

int isMono(void)
{
   BYTE x,y;

   outp(MONO_PORT, 15);
   x = inp(MONO_PORT + 1);
   outp(MONO_PORT + 1, 0x66);
   iodelay(2000);
   y = inp(MONO_PORT + 1);
   outp(MONO_PORT + 1, x);

   return y == 0x66;
}

SEL selMonoBuf;

void MonoInit(void)
{
   if (!isMono())
      return;

   fIsMono = TRUE;

   if (DevHelp_PhysToUVirt(0xB0000, 4096, SELTYPE_R3DATA, 0, &pusMonoBuf3))
      return;

   if (DevHelp_AllocGDTSelector(&selMonoBuf, 1))
      return;

   if (DevHelp_PhysToGDTSelector(0xB0000, 4096, selMonoBuf))
      return;

   pusMonoBuf0 = (USHORT __far *) (((ULONG) selMonoBuf) << 16);
   pusMonoBuf = pusMonoBuf3;
}

void MonoWrite(char __far *psz)
{
   static USHORT usCursor = 0;
   char b;

   if (!fIsMono)
      return;

   if (!fRing3 && pusMonoBuf3) {
      DevHelp_PhysToUVirt((unsigned long) pusMonoBuf3, 4096, SELTYPE_FREE, 0, NULL);
      pusMonoBuf3 = NULL;
      pusMonoBuf = pusMonoBuf0;
   }

   while (*psz) {
      if (usCursor >= 0x2000) {
         _fmemcpy(pusMonoBuf, pusMonoBuf+80, 0x1F80);
         usCursor = 0x1f80;
      }

      b = *psz;
      switch (b) {
         case '\n':
            usCursor = ((usCursor + 79) / 80) * 80;
            break;
         case '\r':
            break;
         case '\t':
            usCursor = ((usCursor + 7) / 8) * 8;
            break;
         default:
            pusMonoBuf3[usCursor++] = MAKEUSHORT(b, 7);
      }
      psz++;
   }

}

