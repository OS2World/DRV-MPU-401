rem @echo off
setlocal
set INCLUDE=%WATCOM%\H
set PATH=%WATCOM%\BINP;%WATCOM%\BINW;%WATCOM%\BINB;%PATH%
wdw %1 %2 %3
endlocal
