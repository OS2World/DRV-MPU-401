/* IDC.H

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
*/

#ifndef OS2_INCLUDED
#define INCL_NOPMAPI
#include <os2.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

USHORT __far __loadds __cdecl Open(USHORT, USHORT);
USHORT __far __loadds __cdecl Close(USHORT, USHORT);
USHORT __far __loadds __cdecl RecvString(USHORT, BYTE __far *, USHORT);
USHORT __far __loadds __cdecl RecvByte(USHORT, BYTE);
void __far __loadds __cdecl IOCtl(PIOCTL_RP pioc);

#ifdef __cplusplus
}
#endif

