@ECHO OFF
setlocal

  REM  This variable sets the com port of your host system and the baud rate at
  REM  which ICAT communicates w/ KDB.  Note that at speeds > 19200, you will
  REM  need buffered UARTs on both the host and victim machines.

  SET  CAT_MACHINE=COM2:115200

  REM  This variable is used by the iprobe library to start initial
  REM  communication w/ KDB.  If it's not specified, the iprobe library defaults
  REM  to 9600.  In most cases, you do NOT need to set this variable.  The
  REM  iprobe library will try to communicate initially w/ KDB at this rate, and
  REM  if that fails, it will try to communicate at the CAT_MACHINE baud rate
  REM  (the rate at which you want to have ICAT and KDB communicating).  We
  REM  invented this variable in case you were previously communicating w/ KDB
  REM  (say using zoc or t) at a rate different from 9600 or the CAT_MACHINE
  REM  rate.
  REM
  REM  This variable is important, however, when ICAT is closed down.  If ICAT
  REM  has this variable set, it will reset KDB to this rate on shut down.
  REM  This facilitates those who want ICAT to communicate at the higher baud
  REM  rates yet want to use terminal emulators before and after that can not
  REM  handle the higher baud rates.

  SET  CAT_SETUP_RATE=9600

  REM  This variable tells ICAT where to find your debug binaries (the .sys
  REM  and .exe files w/ debug information) on your host system.  You should
  REM  put doscalls.cvk (renamed to OS2KRNL) in this host machine subdir IFF
  REM  you are debugging OS/2 kernel code.  Ensure that the version of
  REM  doscalls.cvk lines up w/ the OS2KRNLD that you boot on the victim
  REM  machine, please!

  SET  CAT_HOST_BIN_PATH=n:\base\src\dev\mme\smp401;n:\base\src\dev\mme\midi;n:\base\src\vdev\mme\vmpu401

  REM  This variable is for async communications.  W/ the uKernel, we could
  REM  use token ring and ethernet, too.  Maybe someday w/ Merlin we can.  For
  REM  now, use ASYNC_SIGBRK.

  SET  CAT_COMMUNICATION_TYPE=ASYNC_SIGBRK

  REM  This variable tells ICAT where to find your source (SAMPLEDD.ASM and
  REM  SAMPLE.C) in the demo.  If you are debugging the kernel, ensure that
  REM  your source paths are included here for the kernel components of
  REM  interest.

  SET  CAT_HOST_SOURCE_PATH=%CAT_HOST_BIN_PATH%;n:\base\src\dev\mme\wpddlib;n:\base\h

  REM  This variable causes a recursive search of the subdirectories below the
  REM  subdirectories listed in CAT_HOST_BIN_PATH and CAT_HOST_SOURCE_PATH.
  REM  For example, with the CAT_HOST_SOURCE_PATH variable above, ICAT will
  REM  search the samdetw subdirectory and all subdirectories below samdetw as
  REM  well as their subdirectories, ad nauseam.  This variable defaults to NULL
  REM  so that ICAT will NOT do the recursive search.  Setting this variable to
  REM  any non-null value causes the recursive search to be performed.

  SET  CAT_PATH_RECURSE=

  REM  This variable allows ICAT to disregard all modules other than OS2KRNL and
  REM  the ones specified in the string.  If this variable is null, the iprobe
  REM  library will return all modules to ICAT.  ICAT takes time processing each
  REM  module, so it is often a good idea for performance to supply this
  REM  variable.
  REM
  REM  You can separate the various modules w/ a ';' or a ' '.  (Actually, use
  REM  any delimiter you want as the iprobe library uses strstr on the list to
  REM  see if it should tell ICAT about a module or not.)  The module name must
  REM  be the full name; e.g., mmpmcrts.dll instead of just mmpmcrts.

  SET  CAT_MODULE_LIST=mpu401.sys vmpu401.sys midi.sys

  REM  This variable allows ICAT to resume the system on its initialization.
  REM  You will rarely (if ever) want to do this.  When you invoke icatgam, it
  REM  initializes the COMx port and breaks into KDB.  Then it changes the baud
  REM  rate (when necessary), and it leaves control of KDB in icatgam's hands.
  REM
  REM  This is really important if you are attaching to a system that has
  REM  already reached a failure or has an embedded int3 (device drivers often
  REM  do).  As such, the default is for CAT_RESUME to be NULL.
  REM
  REM  There could be situations where you want to initialize icatgam, but
  REM  somehow time your attach to the OS2KRNL via the [Attach] option.  In
  REM  this rare case, you may want to set CAT_RESUME=ON, and then icatgam will
  REM  resume the victim system waiting for your attach command to stop it
  REM  again.

  SET  CAT_RESUME=

  REM  This variable allows ICAT to configure KDB with some of your favorite
  REM  commands.  An example would be CAT_KDB_INIT=vsf *     .  This would have
  REM  KDB trap back to ICAT when a fatal trap/fault occurs in a ring-3 app.
  REM  You can use multiple KDB commands separated by semicolons in the string.

  SET  CAT_KDB_INIT=

  REM  This variable allows ICAT to disconnect from debugee without restarting
  REM  debugee. This is useful reattaching to debuggee but not have the debuggee
  REM  run between attaches.

  SET  CAT_KEEP_KDB_ON_DETACH=

  REM  If either of the next two environment variables is defined, icatgam sets
  REM  up the COMx port provided by the CAT_MACHINE environment variable so
  REM  that icatgam can talk to a modem and issues the modem attention string
  REM  (+++).  If CAT_MODEM_INIT is defined, icatgam then sends the modem the
  REM  string contained in the CAT_MODEM_INIT environment variable.  If
  REM  CAT_DIAL is defined, icatgam then sends the modem the string contained
  REM  in the CAT_DIAL environment variable and waits 500 seconds for a
  REM  connection to be established.
  REM
  REM  Our intention is that you will figure out the magic AT commands for
  REM  CAT_MODEM_INIT based on whatever you currently use in your terminal
  REM  emulator (which presumably doesn't change very often).  You can then
  REM  leave CAT_MODEM_INIT alone and set CAT_DIAL to the command to actually
  REM  place the call (which presumably changes frequently).

  REM  SET  CAT_DIAL=ATDT4840

  REM  SET  CAT_MODEM_INIT=ATZ

rem call l:\bin\setenv.cmd
call c:\icat\bin\setenv.cmd
del c:\os2\*.@*
icatgam
endlocal
