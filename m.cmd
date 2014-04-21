@echo off
setlocal
rem if "%WATCOM%" = ""
rem SET WATCOM=N:\BASE\WATCOM
set BEGINLIBPATH=..\..\..\..\watcom\binp\dll
set PATH=..\..\..\..\watcom\binp;%PATH%
wmake %1 %2 %3 %4 %5 %6 %7 %8 %9
endlocal

