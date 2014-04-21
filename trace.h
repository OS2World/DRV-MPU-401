/* TRACE.H
*/

#ifndef TRACE_INCLUDED
#define TRACE_INCLUDED

#ifndef __32BIT__          // we need this for the Watcom compiler
#ifdef __386__
#define __32BIT__
#endif
#endif

#include "constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TRACE_EVENT {
   unsigned long ulEvent;
   unsigned long ulParameter;
} TRACE_EVENT;


#define TRACE_STRATEGY                 0x100
#define TRACE_STRATEGY_OPEN            (TRACE_STRATEGY + 1)
#define TRACE_STRATEGY_CLOSE           (TRACE_STRATEGY + 2)
#define TRACE_STRATEGY_IOCTL_INIT      (TRACE_STRATEGY + 4)
#define TRACE_STRATEGY_IOCTL_CTRL      (TRACE_STRATEGY + 5)
#define TRACE_STRATEGY_IOCTL_CAPS      (TRACE_STRATEGY + 6)

#define TRACE_DDCMD                    0x200
#define TRACE_DDCMD_SETUP              (TRACE_DDCMD + 1)
#define TRACE_DDCMD_READ               (TRACE_DDCMD + 2)
#define TRACE_DDCMD_WRITE              (TRACE_DDCMD + 3)
#define TRACE_DDCMD_STATUS             (TRACE_DDCMD + 4)
#define TRACE_DDCMD_REGISTER           (TRACE_DDCMD + 5)
#define TRACE_DDCMD_DEREG              (TRACE_DDCMD + 6)
#define TRACE_DDCMD_CNTRL_START        (TRACE_DDCMD + 7)
#define TRACE_DDCMD_CNTRL_STOP         (TRACE_DDCMD + 8)
#define TRACE_DDCMD_CNTRL_PAUSE        (TRACE_DDCMD + 9)
#define TRACE_DDCMD_CNTRL_RESUME       (TRACE_DDCMD + 10)
#define TRACE_DDCMD_CNTRL_EVENT_ON     (TRACE_DDCMD + 11)
#define TRACE_DDCMD_CNTRL_EVENT_OFF    (TRACE_DDCMD + 12)
#define TRACE_DDCMD_CNTRL_PAUSE_TIME   (TRACE_DDCMD + 13)
#define TRACE_DDCMD_CNTRL_RESUME_TIME  (TRACE_DDCMD + 14)

#define TRACE_TIMER0                   0x300
#define TRACE_TIMER0_ENABLE            (TRACE_TIMER0 + 1)
#define TRACE_TIMER0_DISABLE           (TRACE_TIMER0 + 2)

#define TRACE_MIDISTREAM               0x400
#define TRACE_MIDISTREAM_INIT          (TRACE_MIDISTREAM + 1)
#define TRACE_MIDISTREAM_START         (TRACE_MIDISTREAM + 2)
#define TRACE_MIDISTREAM_STOP          (TRACE_MIDISTREAM + 3)
#define TRACE_MIDISTREAM_PAUSE         (TRACE_MIDISTREAM + 4)
#define TRACE_MIDISTREAM_RESUME        (TRACE_MIDISTREAM + 5)

#define TRACE_MPU                      0x500
#define TRACE_MPU_INIT                 (TRACE_MPU + 1)
#define TRACE_MPU_START                (TRACE_MPU + 2)
#define TRACE_MPU_STOP                 (TRACE_MPU + 3)
#define TRACE_MPU_PAUSE                (TRACE_MPU + 4)
#define TRACE_MPU_RESUME               (TRACE_MPU + 5)
#define TRACE_MPU_SILENCE              (TRACE_MPU + 6)

#define TRACE_VDD                      0x600
#define TRACE_VDD_PDD_INIT             (TRACE_VDD + 1)
#define TRACE_VDD_PDD_QUERY            (TRACE_VDD + 2)
#define TRACE_VDD_PDD_HANDLE           (TRACE_VDD + 3)
#define TRACE_VDD_PDD_OPEN             (TRACE_VDD + 4)
#define TRACE_VDD_PDD_CLOSE            (TRACE_VDD + 5)
#define TRACE_VDD_PDD_INFO             (TRACE_VDD + 6)

#define TRACE_MPUHW                    0x700
#define TRACE_MPUHW_READ               (TRACE_MPUHW + 1)
#define TRACE_MPUHW_WRITE              (TRACE_MPUHW + 2)
#define TRACE_MPUHW_CMD                (TRACE_MPUHW + 3)
#define TRACE_MPUHW_ISR                (TRACE_MPUHW + 4)

#ifndef __386__

int TraceRead(TRACE_EVENT __far *);
#ifdef DEBUG
void Trace(unsigned long ulEvent, unsigned long ulParameter);
#else
#define Trace(a,b)
#endif

#endif

typedef struct _STATUS {
   USHORT usLength;
   USHORT fSMP;
   USHORT fMCA;
   USHORT usNumMPUs;
   USHORT ausIOPorts[MAX_MPU401];
   USHORT ausIRQs[MAX_MPU401];
   USHORT fQuietInit;
   USHORT usMasterVolume;
   USHORT fInUse;
   USHORT usNumStreams;
} STATUS;

extern STATUS status;

#ifdef __cplusplus
}
#endif

#endif
