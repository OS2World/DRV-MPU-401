/* END.H

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
*/

#ifndef END_INCLUDED
#define END_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

extern int end_of_data;
extern int end_of_initdata;

extern unsigned uMemFree;
extern char mbFirstFree;
extern char mbFirstUsed;
extern char end_of_heap;

void end_of_text(void);

#ifdef __cplusplus
}
#endif

#endif
