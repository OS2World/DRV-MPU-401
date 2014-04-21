/* IDC.C - IDC Entry points that MIDI.SYS calls

   MODIFICATION HISTORY
   DATE       PROGRAMMER   COMMENT
   01-Jul-95  Timur Tabi   Creation
   01-Jan-96  Timur Tabi   Sends reset to hardware upon Close
   20-Nov-96  Timur Tabi   Removed #include "..\printf\printf.h"
*/

#define INCL_NOPMAPI
#include <os2.h>

#include <include.h>

#include "constants.h"
#include "isr.hpp"
#include "mpu401.hpp"

unsigned int fOpenedSend[MAX_MPU401]={FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE};
unsigned int fOpenedRecv[MAX_MPU401]={FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE};

/***************************************************************************
   Function: Open
   Purpose: Opens the MPU-401 for either send or receive
   Parameters:
      usPort - index into array
      usMode - either MIDIMODE_OPEN_RECEIVE or MIDIMODE_OPEN_SEND
   Purpose:
      This funcitons initializes (opens) the MPU-401 for either send or receive.
      It first checks to see if it's been opened for that funtion already.  This
      should not happen (the MIDI driver doesn't send two consecutive opens for
      the same function), but just in case it just returns success.
      It then checks to see if the hardware has already been initialized.  If
      so, then the driver doesn't bother initializing it again.
      If it's initialized for send, then the driver also tries to take the IRQ.
*/

extern "C" USHORT __far __loadds __cdecl Open(MPU *pmpu, USHORT usMode)
{
   USHORT us;

   switch (usMode) {
      case MIDIMODE_OPEN_RECEIVE:
         if (fOpenedRecv[pmpu->port()]) return 0;     // opened for receive already?
         if (!fOpenedSend[pmpu->port()]) {            // not opened for send either?
            if (!pmpu->init())                        // then initialize it
               return MIDIERRA_HW_FAILED;             // hmmm... initialize failed
         }
         fOpenedRecv[pmpu->port()]=TRUE;
         return 0;
      case MIDIMODE_OPEN_SEND:
         if (fOpenedSend[pmpu->port()]) return 0;     // opened for send already?
         if (!fOpenedRecv[pmpu->port()]) {            // not opened for receive either?
            if (!pmpu->init())                        // then initialize it
               return MIDIERRA_HW_FAILED;             // hmmm... initialize failed
         }
         if (!pmpu->irq()->enable())                 // Couldn't grab the IRQ
            return MIDIERRA_CANT_GET_IRQ;

         fOpenedSend[pmpu->port()]=TRUE;
         return 0;
   }

// Invalid mode requested, so just return general failure
   return MIDIERRA_GEN_FAILURE;
}

/***************************************************************************
   Function: Close
   Purpose: Closes the MPU-401 for either send or receive
   Parameters:
      pmpu->port() - index into array
      usMode - either MIDIMODE_OPEN_RECEIVE or MIDIMODE_OPEN_SEND
   Purpose:
      This function closes the MPU-401.  It doesn't really do anything to the
      hardware, although maybe it should reset it back to intelligent mode.
      If the close is for sending, then it detaches the IRQ.
*/

extern "C" USHORT __far __loadds __cdecl Close(MPU *pmpu, USHORT usMode)
{
   switch (usMode) {
      case MIDIMODE_OPEN_RECEIVE:
         fOpenedRecv[pmpu->port()]=FALSE;
         pmpu->command(0xFF);
         return 0;
      case MIDIMODE_OPEN_SEND:
         pmpu->irq()->disable();
         fOpenedSend[pmpu->port()] = FALSE;
         pmpu->command(0xFF);
         return 0;
   }

// Invalid mode requested, so just return general failure
   return MIDIERRA_GEN_FAILURE;
}

/***************************************************************************
   Function: RecvByte
   Purpose: Receives a single byte of MIDI data from the MIDI driver
   Parameters:
      pmpu->port() - index into array
      b - the byte
   Purpose:
      The MIDI driver calls this function to send a single byte of MIDI data.
      If this function returns an error code, the MIDI driver will probable close
      the device.
*/

extern "C" USHORT __far __loadds __cdecl RecvByte(MPU *pmpu, BYTE b)
{
   return MPUwrite(pmpu->port(),b) ? 0 : MIDIERRA_HW_FAILED;
}

/***************************************************************************
   Function: RecvString
   Purpose: Receives a string (not null-terminated) of MIDI data from the MIDI driver
   Parameters:
      pmpu->port() - index into array
      pb - far pointer to the string
      usLength - length of that string
   Purpose:
      The MIDI driver calls this function to send a string of MIDI data.  Typically,
      this will be used only for large chunks of bytes, such as a SysEx message.
      If this function returns an error code, the MIDI driver will probable close
      the device.
*/

extern "C" USHORT __far __loadds __cdecl RecvString(MPU *pmpu, BYTE __far *pb, USHORT usLength)
{
   while (usLength--)
      if (!MPUwrite(pmpu->port(),*pb++))
         return MIDIERRA_HW_FAILED;

   return 0;
}

/***************************************************************************
   Function: IOCtl
   Purpose: Receives an MMPM/2 IOCtl
   Parameters:
      pioc - a far pointer to the MMPM/2 IOCtl request packet
   Purpose:
      The MIDI driver may, in a future release, act as a bridge between the
      type A drivers and MMPM/2.  When operating in this mode, MMPM/2 will send
      miscellaneous mixer-related IOCtls to the MIDI driver itself, instead of
      to the type A driver.  To remedy this, the MIDI driver will redirect these
      IOCtls to the the type A driver via this function call.
*/

#pragma off (unreferenced)

extern "C" void __far __loadds __cdecl IOCtl(PIOCTL_RP pioc)
{
   // This prototype is included for reference only.  The MMPM/2 bridge has
   // not yet been defined, and this particular driver wouldn't use it anyway.
}
