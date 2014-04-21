/* MPUTRACE.CPP
*/

#include <stdio.h>
#include <conio.h>
#include <string.h>

#define INCL_NOPMAPI
#define INCL_DOSFILEMGR
#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_DOSPROCESS
#include <os2.h>

#include "trace.h"

#define OPEN_FLAG ( OPEN_ACTION_OPEN_IF_EXISTS )
#define OPEN_MODE ( OPEN_FLAGS_FAIL_ON_ERROR | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE )

HFILE hfile = 0;
unsigned int fQuit = FALSE;
unsigned int fRunning = FALSE;

typedef struct _TRACE_STRING {
   ULONG ulId;
   char *psz;
} TRACE_STRING;

STATUS status;

TRACE_STRING ats[] =
{
   {TRACE_STRATEGY, "TRACE_STRATEGY"},
   {TRACE_STRATEGY_OPEN, "TRACE_STRATEGY_OPEN" },
   {TRACE_STRATEGY_CLOSE, "TRACE_STRATEGY_CLOSE" },
   {TRACE_STRATEGY_IOCTL_INIT, "TRACE_STRATEGY_IOCTL_INIT" },
   {TRACE_STRATEGY_IOCTL_CTRL, "TRACE_STRATEGY_IOCTL_CTRL" },
   {TRACE_STRATEGY_IOCTL_CAPS, "TRACE_STRATEGY_IOCTL_CAPS" },

   {TRACE_DDCMD, "TRACE_DDCMD" },
   {TRACE_DDCMD_SETUP, "TRACE_DDCMD_SETUP" },
   {TRACE_DDCMD_READ, "TRACE_DDCMD_READ" },
   {TRACE_DDCMD_WRITE, "TRACE_DDCMD_WRITE" },
   {TRACE_DDCMD_STATUS, "TRACE_DDCMD_STATUS" },
   {TRACE_DDCMD_REGISTER, "TRACE_DDCMD_REGISTER" },
   {TRACE_DDCMD_DEREG, "TRACE_DDCMD_DEREG" },
   {TRACE_DDCMD_CNTRL_START, "TRACE_DDCMD_CNTRL_START" },
   {TRACE_DDCMD_CNTRL_STOP, "TRACE_DDCMD_CNTRL_STOP" },
   {TRACE_DDCMD_CNTRL_PAUSE, "TRACE_DDCMD_CNTRL_PAUSE" },
   {TRACE_DDCMD_CNTRL_RESUME, "TRACE_DDCMD_CNTRL_RESUME" },
   {TRACE_DDCMD_CNTRL_EVENT_ON, "TRACE_DDCMD_CNTRL_EVENT_ON" },
   {TRACE_DDCMD_CNTRL_EVENT_OFF, "TRACE_DDCMD_CNTRL_EVENT_OFF" },
   {TRACE_DDCMD_CNTRL_PAUSE_TIME, "TRACE_DDCMD_CNTRL_PAUSE_TIME" },
   {TRACE_DDCMD_CNTRL_RESUME_TIME, "TRACE_DDCMD_CNTRL_RESUME_TIME" },

   {TRACE_TIMER0, "TRACE_TIMER0" },
   {TRACE_TIMER0_ENABLE, "TRACE_TIMER0_ENABLE" },
   {TRACE_TIMER0_DISABLE, "TRACE_TIMER0_DISABLE" },

   {TRACE_MIDISTREAM, "TRACE_MIDISTREAM" },
   {TRACE_MIDISTREAM_INIT, "TRACE_MIDISTREAM_INIT" },
   {TRACE_MIDISTREAM_START, "TRACE_MIDISTREAM_START" },
   {TRACE_MIDISTREAM_STOP, "TRACE_MIDISTREAM_STOP" },
   {TRACE_MIDISTREAM_PAUSE, "TRACE_MIDISTREAM_PAUSE" },
   {TRACE_MIDISTREAM_RESUME, "TRACE_MIDISTREAM_RESUME" },

   {TRACE_MPU, "TRACE_MPU" },
   {TRACE_MPU_INIT, "TRACE_MPU_INIT" },
   {TRACE_MPU_START, "TRACE_MPU_START" },
   {TRACE_MPU_STOP, "TRACE_MPU_STOP" },
   {TRACE_MPU_PAUSE, "TRACE_MPU_PAUSE" },
   {TRACE_MPU_RESUME, "TRACE_MPU_RESUME" },
   {TRACE_MPU_SILENCE, "TRACE_MPU_SILENCE" },

   {TRACE_VDD, "TRACE_VDD" },
   {TRACE_VDD_PDD_INIT, "TRACE_VDD_PDD_INIT" },
   {TRACE_VDD_PDD_QUERY, "TRACE_VDD_PDD_QUERY" },
   {TRACE_VDD_PDD_HANDLE, "TRACE_VDD_PDD_HANDLE" },
   {TRACE_VDD_PDD_OPEN, "TRACE_VDD_PDD_OPEN" },
   {TRACE_VDD_PDD_CLOSE, "TRACE_VDD_PDD_CLOSE" },
   {TRACE_VDD_PDD_INFO, "TRACE_VDD_PDD_INFO" },

   {TRACE_MPUHW, "TRACE_MPUHW" },
   {TRACE_MPUHW_READ, "TRACE_MPUHW_READ" },
   {TRACE_MPUHW_WRITE, "TRACE_MPUHW_WRITE" },
   {TRACE_MPUHW_CMD, "TRACE_MPUHW_CMD" },
   {TRACE_MPUHW_ISR, "TRACE_MPUHW_ISR" }
};

#define NUM_TRACES (sizeof(ats) / sizeof(ats[0]))


void PrintStatus(void)
{
   unsigned i;

   printf("usLength: %d\n", status.usLength);
   printf("fSMP: %d\n", status.fSMP);
   printf("fMCA: %d\n", status.fMCA);
   printf("usNumMPUs: %d\n", status.usNumMPUs);
   if (status.usNumMPUs > MAX_MPU401)
      status.usNumMPUs = MAX_MPU401;
   printf("ausIOPorts: ");
   for (i=0; i<status.usNumMPUs; i++)
      printf("%4x ", status.ausIOPorts[i]);
   printf("\nausIRQs: ");
   for (i=0; i<status.usNumMPUs; i++)
      printf("%4x ", status.ausIRQs[i]);
   printf("\nfQuietInit: %d\n", status.fQuietInit);
   printf("usMasterVolume: %d\n", status.usMasterVolume);
   printf("fInUse: %d\n", status.fInUse);
   printf("usNumStreams: %d\n", status.usNumStreams);
}

void ShowTrace(TRACE_EVENT &te)
{
   unsigned i;

   for (i=0; i<NUM_TRACES; i++)
      if (ats[i].ulId == te.ulEvent) {
         printf("%s (%lu 0x%lx)\n", ats[i].psz, te.ulParameter, te.ulParameter);
         return;
      }

   printf("Unknown message %lu 0x%lx (%lu 0x%lx)\n", te.ulEvent, te.ulParameter, te.ulEvent, te.ulParameter);
}

VOID APIENTRY Thread(VOID)
{
   APIRET rc;
   ULONG ulNumBytes;
   TRACE_EVENT te;

   printf("Thread() started\n");
   rc = DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, 0);
   if (rc) {
      printf("Thread() DosSetPriority failed with RC = %lu\n", rc);
      return;
   }

   fRunning = TRUE;
   while (!fQuit) {
      rc = DosRead(hfile, &te, sizeof(TRACE_EVENT), &ulNumBytes);
      if (rc) {
         printf("Error %d while calling device driver\n", rc);
         return;
      }
      ShowTrace(te);
   }

   printf("Thread() stopped\n");
   fRunning = FALSE;
}

void main(int argc, char **argv)
{
   APIRET rc;
   ULONG ulAction = 0;
   TID tid = 0;
   char szTemp[] = "NPU4011$";
   char szDevName[128] = "\\DEV\\";

   if (argc > 1) {
      strcpy(szTemp, argv[1]);
      szTemp[0]++;
   }

   strcat(szDevName, szTemp);

   printf("MPUTRACE - Theta Band Software MPU-401 Driver Trace Utility\n");
   printf("%d Trace Messages supported\n", NUM_TRACES);
   printf("Opening %s\n", szDevName);
   rc = DosOpen((PSZ) szDevName, &hfile, &ulAction, 0, 0, OPEN_FLAG, OPEN_MODE, NULL);
   if (rc) {
      printf("Error %d while trying to open MPU-401 device driver\n", rc);
      return;
   }

   ULONG ulSize1;
   ULONG ulSize2 = sizeof(STATUS);
   rc = DosDevIOCtl(hfile, 0x80, 0, NULL, ulSize1, &ulSize1, &status, ulSize2, &ulSize2);
   if (rc) {
      printf("Error %d while trying to get MPU-401 status\n", rc);
      return;
   }

   PrintStatus();

   printf("Creating thread\n");
   rc = DosCreateThread(&tid, (PFNTHREAD) Thread, 0, CREATE_READY | STACK_COMMITTED, 8192);
   if (rc) {
      printf("DosCreateThread failed with RC = %lu\n", rc);
      return;
   }

   while (!fRunning)
      DosSleep(100);

   printf("Hit any key to quit. Program will then exit after the next message.\n");

   getch();
   printf("Quitting ...\n");
   fQuit = TRUE;
   while (fRunning)
      DosSleep(100);

   DosSleep(500);    // wait until the thread really ends
   DosClose(hfile);
}
