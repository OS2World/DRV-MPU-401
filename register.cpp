/* REGISTER.CPP
*/

#pragma code_seg ("_inittext");
#pragma data_seg ("_initdata","endds");

#include <include.h>

#define KEYLEN   128

#pragma align
char abKey[KEYLEN+1] =
   {"REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_REGISTRATIONKEY_"};

unsigned fValidKey = FALSE;

unsigned VerifyKey(void)
{
   unsigned i=0;
   USHORT usToken = 0;

   for (i=0; i<KEYLEN; i++)
      abKey[i] = 256 - abKey[i];

   for (i=0; i<KEYLEN; i+=2)
      usToken += * ((USHORT *) (abKey+i));

   fValidKey = usToken == 0;
   return fValidKey;
}
