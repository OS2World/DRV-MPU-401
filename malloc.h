/* MALLOC.H

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
   01-Apr-96  Timur Tabi   Fixed realloc and _msize prototypes
*/

#ifndef MALLOC_INCLUDED
#define MALLOC_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// Standard malloc.h functions

void __near *malloc(unsigned);
void free(void __near *);
void __near *realloc(void __near *, unsigned);
unsigned _msize(void __near *);

// Some extensions
unsigned _memfree(void);            // returns available space

#ifdef __cplusplus
}
#endif

#endif
