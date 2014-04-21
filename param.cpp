/* PARAM.CPP - Parses command-line parameters
*/

#pragma code_seg ("_inittext");
#pragma data_seg ("_initdata","endds");

#include <ctype.h>
#include <string.h>

#include "param.hpp"
#include "rmhelp.hpp"
#include "mpu401.hpp"
#include "vdd.hpp"
#include <include.h>

// The MPU-401 base I/O ports specified on the command line.
USHORT ausCL_BaseIO[MAX_MPU401] = {0,0,0,0,0,0,0,0,0};

// The IRQ levels specified on the command line
USHORT ausCL_IRQ[MAX_MPU401] = {0,0,0,0,0,0,0,0,0};

// The device header name specified on the command line
char szCL_DevName[8] = {' ',' ',' ',' ',' ',' ',' ',' '};

// TRUE if /P and /I parameters were specified on the command line
unsigned int fParamsSpecified = FALSE;

// True if the /V parameter was specified
extern "C" unsigned int fVerbose;

// if True, issue an INT3 at the beginning of the INIT section
unsigned int fInt3 = FALSE;

// True if Resource Manager I/O checks should be ignored (/O:NORMIO)
unsigned int fNoRMIO = FALSE;

// True if Resource Manager IRQ checks should be ignored (/O:NORMIRQ)
unsigned int fNoRMIRQ = FALSE;

// True if the I/O ranges shouldn't be checked (/O:NOCHECKIO)
// Note that if fNoCheckIO is TRUE, then we can't autodetect an IRQ
// because we need to do port I/O to find the IRQ.
unsigned int fNoCheckIO = FALSE;

// True if the IRQ's shouldn't be autodetected (/O:NOCHECKIRQ)
unsigned int fNoCheckIRQ = FALSE;

// True if we should not use TIMER0 even if we find it
unsigned int fNoTimer0 = FALSE;

// The value of the /R parameter
int iCL_Resolution = 2;

// declared in strategy.c
// TRUE if we should use long RTMIDI names (w/ I/O port and IRQ)
extern unsigned int fLongNames;

// TRUE if we should only use the 1st MPU-401 that we find
unsigned int fOnlyOne = FALSE;

// The number of address bits reported to RM
// Changed to 16 if /O:16BITS is specified or if MicroChannel
// The default used to be 10, but most modern devices do 16
USHORT usRMWordSize = 16;

// The flags used for the I/O resource allocation
// Changed to RS_IO_SHARED if /O:SHAREDIO is specified
// Changed to RS_IO_MULTIPLEXED if /O:MULTIIO is specified
USHORT usRMIOFlag = RS_IO_EXCLUSIVE;

// The flags used for the IRQ resource allocation
// Changed to RS_IRQ_SHARED if /O:SHAREDIRQ is specified
// Changed to RS_IRQ_MULTIPLEXED if /O:MULTIIRQ is specified
USHORT usRMIRQFlag = RS_IRQ_EXCLUSIVE;

USHORT sz2us(char __far *sz, int base)
{
   static char digits[] = "0123456789ABCDEF";

   USHORT us = 0;
   char *pc;

// skip leading spaces
   while (*sz == ' ') sz++;

// skip leading zeros
   while (*sz == '0') sz++;

// accumulate digits - return error if unexpected character encountered
   for (;;sz++) {
      pc = (char *) memchr(digits, toupper(*sz), base);
      if (!pc)
         return us;
      us = (us * base) + (pc - digits);
   }
}

int IsWhitespace(char ch)
{
   if ( ch > '9' && ch < 'A')
      return TRUE;
   if ( ch < '0' || ch > 'Z')
      return TRUE;

   return FALSE;
}

char __far *SkipWhite(char __far *psz)
{
   while (*psz) {
      if (!IsWhitespace((char) toupper(*psz))) return psz;
      psz++;
   }
   return NULL;
}

int CopyDevicename(char __far *psz)
{
   int i,j;
   char ch;

// first, check if the filename is valid
   for (i = 0; i<9; i++) {
      ch=(char) toupper(psz[i]);
      if (!ch || ch == ' ')
         break;
      if (i==8)                           // too long?
         return FALSE;
      if ( ch > '9' && ch < 'A')
         return FALSE;
      if ( (ch != '$' && ch < '0') || ch > 'Z')
         return FALSE;
   }
   if (!i) return FALSE;                        // zero-length name?

   for (j = 0; j<i; j++)
      szCL_DevName[j]=(char) toupper(psz[j]);

   for (; j<8; j++)
      szCL_DevName[j]=' ';

   return TRUE;
}

#define OPTION(sz)   (!_fstrnicmp(pszOption, sz, sizeof(sz)-1))

int DoOption(char __far *pszOption)
{
   if OPTION("LONGNAME") {
      fLongNames = TRUE;
      return TRUE;
   }

   if OPTION("NORMIO") {
      fNoRMIO = TRUE;
      return TRUE;
   }

   if OPTION("SHAREDIO") {
      usRMIOFlag = RS_IO_SHARED;
      return TRUE;
   }

   if OPTION("MULTIIO") {
      usRMIOFlag = RS_IO_MULTIPLEXED;
      return TRUE;
   }

   if OPTION("NORMIRQ") {
      fNoRMIRQ = TRUE;
      return TRUE;
   }

   if OPTION("SHAREDIRQ") {
      usRMIRQFlag = RS_IRQ_SHARED;
      return TRUE;
   }

   if OPTION("MULTIIRQ") {
      usRMIRQFlag = RS_IRQ_MULTIPLEXED;
      return TRUE;
   }

   if OPTION("NOCHECKIO") {
      fNoCheckIO = TRUE;
      fNoCheckIRQ = TRUE;     // see comment for fNoCheckIO
      return TRUE;
   }

   if OPTION("NOCHECKIRQ") {
      fNoCheckIRQ = TRUE;
      return TRUE;
   }

   if OPTION("NOTIMER0") {
      fNoTimer0 = TRUE;
      return TRUE;
   }

   if OPTION("10BITS") {
      usRMWordSize = 10;
      return TRUE;
   }

   if OPTION("16BITS") {
      usRMWordSize = 16;
      return TRUE;
   }

   if OPTION("QUIETINIT") {
      fQuietInit = TRUE;
      return TRUE;
   }

   if OPTION("ONLYONE") {
      fOnlyOne = TRUE;
      return TRUE;
   }

   if OPTION("INT3") {
      fInt3 = TRUE;
      return TRUE;
   }

   if OPTION("AUDIOVDD") {
      fEnableVDD = TRUE;
      return TRUE;
   }

   return FALSE;
}

int DoParm(char cParm, int iPort, char __far *pszOption)
{
   USHORT us;

   switch (cParm) {
      case 'I':                     // IRQ
         if (!iPort) return FALSE;
         if (!pszOption) return FALSE;
         us = sz2us(pszOption, 10);
         if (us > 15) return FALSE;
         if (us)
            ausCL_IRQ[iPort-1] = us;
         else
            ausCL_IRQ[iPort-1] = -1;
         fParamsSpecified = TRUE;
         break;
      case 'N':                     // device header name
         if (iPort) return FALSE;
         if (!pszOption) return FALSE;
         if (!CopyDevicename(pszOption)) return FALSE;
         break;
      case 'O':                     // miscellaneous options
         if (iPort) return FALSE;
         if (!pszOption) return FALSE;
         return DoOption(pszOption);
      case 'P':                     // Base port I/O address
         if (!iPort) return FALSE;
         if (!pszOption) return FALSE;
         us = sz2us(pszOption, 16);
         if (us) {
            ausCL_BaseIO[iPort-1] = us;
            fParamsSpecified = TRUE;
         }
         break;
      case 'R':                     // MMPM/2 timing resolution
         if (iPort) return FALSE;
         if (!pszOption) return FALSE;
         us = sz2us(pszOption,10);
         if (!us) return FALSE;
         iCL_Resolution = us;
         break;
      case 'V':                     // Verbose option
         if (iPort) return FALSE;
         if (pszOption) return FALSE;
         fVerbose = TRUE;
         break;
      default:
         return FALSE;              // unknown parameter
    }

   return TRUE;
}

/* Function: ParseParm
   Input: pointer to the letter of the parameter (e.g. the 'P' in 'P1:330').
          length of this parameter, which must be at least 1
   Output: TRUE if the parameter was value
   Purpose: parses the string into three parts: the letter parameter, the port
            number, and the option string.  Calls DoParm with these values.
   Notes:
      the following describes the format of valid parameters.
         1. Parameters consist of a letter, an optional number, and an
            optional 'option'.  The format is x[n][:option], where 'x' is the
            letter, 'n' is the number, and 'option' is the option.
         2. Blanks are delimeters between parameters, therefore there are no
            blanks within a parameter.
         3. The option is preceded by a colon
      This gives us only four possibilities:
         P (length == 1)
         P1 (length == 2)
         P:option (length >= 3)
         P1:option (length >= 4)
*/
int ParseParm(char __far *pszParm, int iLength)
{
   char ch,ch1=(char) toupper(*pszParm);       // get letter

   if (iLength == 1)                // only a letter?
      return DoParm(ch1,0,NULL);

   ch = pszParm[1];                 // should be either 1-9 or :
   if (ch < '1' || (ch > '9' && ch != ':'))
      return FALSE;

   if (iLength == 2) {
      if (ch == ':')
         return FALSE;
      return DoParm(ch1,ch - '0',NULL);
   }

   if (iLength == 3) {
      if (ch != ':')
         return FALSE;
      return DoParm(ch1,0,pszParm+2);
   }

   if (ch == ':')
      return DoParm(ch1,0,pszParm+2);

   return DoParm(ch1,ch - '0',pszParm+3);
}

int GetParms(char __far *pszCmdLine)
{
   int iLength;

   while (*pszCmdLine != ' ') {              // skip over filename
      if (!*pszCmdLine) return TRUE;         // no params?  then just exit
      pszCmdLine++;
   }

   while (TRUE) {
      pszCmdLine = SkipWhite(pszCmdLine);      // move to next param
      if (!pszCmdLine) return TRUE;          // exit if no more

      for (iLength = 0; pszCmdLine[iLength]; iLength++)    // calculate length
         if (pszCmdLine[iLength] == ' ') break;          //  of parameter

      if (!ParseParm(pszCmdLine,iLength))    // found parameter, so parse it
         return FALSE;

      while (*pszCmdLine != ' ') {              // skip over parameter
         if (!*pszCmdLine) return TRUE;         // no params?  then just exit
         pszCmdLine++;
      }
   }
}

