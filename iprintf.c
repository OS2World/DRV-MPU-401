/* IPRINTF.C - Init-time printf() support

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   25-Nov-96  Timur Tabi   Creation
*/

#pragma code_seg ("_inittext");
#pragma data_seg ("_initdata","endds");

#define INCL_NOPMAPI
#define INCL_DOSMISC
#include <os2.h>

#include <include.h>
#include "mono.h"

#ifdef DEBUG
unsigned int fVerbose = TRUE;
#else
unsigned int fVerbose = FALSE;
#endif

#define BUFSIZE      128

#define CR 0x0d
#define LF 0x0a

#define LEADING_ZEROES          0x8000

char digits[] = "0123456789ABCDEF";
const USHORT ausPowers[] = {1000,100,10,1,0};
const ULONG aulPowers[] = {100000000,10000000,1000000,100000,10000,1000,100,10,1,0};

char abBuffer[BUFSIZE];
unsigned uIndex = 0;

unsigned long rotl32(unsigned long value, unsigned char count);
#pragma aux rotl32 = \
   "shl  edx,16" \
   "mov  dx,ax" \
   "rol  edx,cl" \
   "mov  ax,dx" \
   "shr  edx,16" \
   value [dx ax] \
   parm nomemory [dx ax] [cl] \
   modify nomemory exact [dx ax];

void Write(char b)
{
   if (uIndex == BUFSIZE)
      return;

   abBuffer[uIndex++]=b;
}

void DecWordToASCII(USHORT usDecVal, USHORT Option)
{
   unsigned int fNonZero = FALSE;
   USHORT bDigit;
   unsigned iPowerIdx = 0;
   USHORT usPower = 10000;

   while (usPower) {
      bDigit = 0;
      while (usDecVal >=usPower ) {                   //bDigit = usDecVal/usPower;
         bDigit++;
         usDecVal-=usPower;
      }

      if (bDigit)
         fNonZero = TRUE;

      if (bDigit || fNonZero || (Option & LEADING_ZEROES) || usPower==1)
         Write((BYTE) ('0'+bDigit));

      usPower=ausPowers[iPowerIdx++];
   }
}

void DecLongToASCII(ULONG ulDecVal, USHORT Option)
{
   unsigned int fNonZero = FALSE;
   BYTE bDigit;
   unsigned iPowerIdx = 0;
   ULONG ulPower=1000000000UL;                       // 1 billion

   while (ulPower) {
      bDigit = 0;                                                                        // bDigit = lDecVal/ulPower
      while (ulDecVal >= ulPower) {                  // replaced with while loop
         bDigit++;
         ulDecVal -= ulPower;
      }

      if (bDigit)
         fNonZero = TRUE;

      if (bDigit || fNonZero || (Option & LEADING_ZEROES) || ulPower==1)
         Write((BYTE) ('0' + bDigit));

      ulPower = aulPowers[iPowerIdx++];
   }
}

void HexWordToASCII(USHORT usHexVal, USHORT Option)
{
   unsigned int fNonZero = FALSE;
   USHORT us=usHexVal;
   BYTE bDigit;
   int i;

   for (i = 0; i<4; i++) {
      us=rotl16(us,4);
      bDigit = us & 0xF;
      if (bDigit)
         fNonZero = TRUE;

      if (bDigit || fNonZero || (Option & LEADING_ZEROES) || i==3)
          Write(digits[bDigit]);
   }
}

void HexLongToASCII(ULONG ulHexVal, USHORT Option)
{
   unsigned int fNonZero = FALSE;
   ULONG ul=ulHexVal;
   BYTE bDigit;
   int i;

   for (i=0; i<8; i++) {
      ul = rotl32(ul,4);
      bDigit = (USHORT) ul & 0xF;
      if (bDigit)
         fNonZero = TRUE;

      if (bDigit || fNonZero || (Option & LEADING_ZEROES) || i==7)
          Write(digits[bDigit]);
   }
}

void iprintf(char *psz , ...)
{
   char *pc = psz;
   char *SubStr;
   union {
      char * __far *ppc;         // points to "char *psz" (format string)
      USHORT __far *pus;         // points to unsigned short parameter
      ULONG __far *pul;          // points to unsigned long parameter
      char * __far *psz;         // points to string parameter
      void __far * __far *pfpv;  // points to far pointer parameter
   } Stack;
   USHORT usBuildOption;

   if (!fVerbose)
      return;

   Stack.ppc = (char * __far *) &psz;
   Stack.ppc++;                            // skip over 1st parameter (format string)

   while (*pc) {
      switch (*pc) {
         case '%':
            usBuildOption = 0;
            pc++;
            if (*pc=='0') {
               usBuildOption = LEADING_ZEROES;
               pc++;
            }

            if (*pc == 'u')                      // always unsigned
               pc++;

            switch(*pc) {
               case 'x':
                  HexWordToASCII(*Stack.pus++, usBuildOption);
                  pc++;
                  break;
               case 'd':
               case 'i':
                  DecWordToASCII(*Stack.pus++, usBuildOption);
                  pc++;
                  break;
               case 's':
                  SubStr = *Stack.psz++;
                  while (*SubStr) Write(*SubStr++);
                  pc++;
                  break;
               case 'l':
                  pc++;
                  switch (*pc) {
                     case 'x':
                        HexLongToASCII(*Stack.pul++, usBuildOption);
                        pc++;
                        break;
                     case 'u':
                     case 'd':
                     case 'i':
                        DecLongToASCII(*Stack.pul++, usBuildOption);
                        pc++;
                        break;
                  }
                  break;
               case 'p':
                  pc++;
                  HexWordToASCII(HIUSHORT(*Stack.pfpv), LEADING_ZEROES);
                  Write(':');
                  HexWordToASCII(LOUSHORT(*Stack.pfpv), LEADING_ZEROES);
                  Stack.ppc++;
                  break;
            }
            break;

         case '\n':
            Write(CR);
            Write(LF);
            pc++;
            break;

         default:
            Write(*pc++);

      } // end switch(*pc)
   } // end while (*pc)

   DosPutMessage(1, uIndex, abBuffer);
   abBuffer[uIndex] = 0;
   MonoWrite(abBuffer);
   uIndex = 0;
}
