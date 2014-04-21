/* HEADER.H - Device Header definitions

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
*/

#ifndef HEADER_INCLUDED
#define HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1);

typedef void (__near *PFNENTRY) (void);

typedef struct {
   unsigned long ulNextDD;
   unsigned short usAttribs;
   PFNENTRY pfnStrategy;
   PFNENTRY pfnIDC;
   char abName[8];
   unsigned long ulReserved[2];
   unsigned long ulCaps;
} DEV_HEADER;

// pseudo-variable that points to device header
#define phdr ((DEV_HEADER *) 0)

#pragma pack();

// Registers the PDD for VDD's. Only valid during INIT time!
int RegisterPDD(PFN);

#ifdef __cplusplus
}
#endif

#endif
