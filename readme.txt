Theta Band Software MPU-401 Driver
Part of the MMPACK

Thank you for purchasing the MMPACK, a collection of multimedia tools
for OS/2 Warp.  This file is an introduction to the MPU-401 driver of
the MMPACK.

This package contains these files:

readme.txt      This file
history.txt     History of changes to the driver
install.cmd     Installation program
mpu401.sys      The actual device driver
vmpu401.sys     The virtual device driver (VDD) used for DOS sharing
cardinfo.dll    Installation DLL
mpu401.dll      Installation DLL
mpu401.hlp      Installation help file
mpu401.inf      Online documentation (same content as help file)
control.scr     Installation script
mpu401.scr      Installation script
midiplay.ico    Installation icon
mputrace.exe    Tracing utility
mputrace.txt    Documentation for MPUTRACE.EXE
rtmlist.exe     RTMIDI status utility
rtmlist.txt     Documentation for RTMLIST.EXE
rtmhwnod.exe    RTMIDI hardware node utility
rtmhwnod.txt    Documentation for RTMHWNOD.EXE

To install this driver, simply run INSTALL.CMD.  The driver will detect
the current installation and act accordingly.

If there are no MPU-401 drivers currently installed, the full-blown
multimedia install program (MINSTALL) will be run.  Online help is
available.

The only limitation is if there are multiple MPU-401 drivers installed,
which typically only happens if you have multiple sound cards.  The
installation program will detect this, and then prompt you to edit your
CONFIG.SYS manually.

Thank you for your support.  We look forward to enhancing the
MPU-401 driver to meet your requirements.  Support for this
driver is provided via the MMPACK mailing list.  If you are
a registered user of the MMPACK and are not on the mailing
list, please email us at support@thetaband.com.

Theta Band Software
http://www.thetaband.com
