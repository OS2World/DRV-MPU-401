#
# Set the environmental variables for compiling
#
.ERASE

.SUFFIXES:
.SUFFIXES: .sys .obj .asm .inc .def .lrf .lst .sym .map .cpp .c .h .lib .dbg .dll .itl .hlp

!loaddll wcc wccd
!loaddll wpp wppdi86

NAME     = mpu401

.BEFORE
!ifndef %WATCOM
   set WATCOM=..\..\..\..\WATCOM
!endif
   set INCLUDE=$(%WATCOM)\H;..\..\RESOURCE\RSM_H;..\..\..\..\H;..\..\..\..\SRC\INC
   set PATH=..\..\..\..\TOOLS;$(%WATCOM)\BINP;$(%WATCOM)\BINW;$(%WATCOM)\BINB
   set LIB=$(%WATCOM)\lib386;$(%WATCOM)\lib386\os2;..\..\..\..\lib

!ifndef %IPFC
   set IPFC=..\..\..\..\TOOLS
!endif

# Options for Watcom 16-bit C compiler
#  -bt=os2   = Build target OS is OS/2
#  -ms       = Memory model small
#  -3        = Enable use of 80386 instructions
#  -4        = Optimize for 486 (implies -3)
#  -5        = Optimize for Pentium (implies -3)
#  -j        = char default is unsigned
#  -d1       = Include line number info in object
#              (necessary to produce assembler listing)
#  -d2       = Include debugging info for ICAT
#              (necessary to produce assembler listing)
#  -o        = Optimization - i = enable inline intrinsic functions
#                             r = optimize for 80486 and pentium pipes
#                             s = space is preferred to time
#                             l = enable loop optimizations
#                             a = relax aliasing constraints
#                             n = allow numerically unstable optimizations
#  -s        = Omit stack size checking from start of each function
#  -zl       = Place no library references into objects
#  -wx       = Warning level set to maximum (vs 1..4)
#  -zfp      = Prevent use of FS selector
#  -zgp      = Prevent use of GS selector
#  -zq       = Operate quietly
#  -zm       = Put each function in its own segment
#  -zu       = Do not assume that SS contains segment of DGROUP
#  -hd       = Include Dwarf debugging info
#  -hc       = Include Codeview debugging info
#
CC=wcc
CPP=wpp
!define COPTS
!define AOPTS

!ifdef DEBUG
!define EXPIRE
COPTS=$+ -oi -dDEBUG
AOPTS=$+ -dDEBUG
!else
COPTS=$+ -olian
!endif

!ifdef EXPIRE
COPTS=$+ $(COPTS) -dEXPIRE
AOPTS=$+ $(AOPTS) -dEXPIRE
!endif

!ifdef SMP
COPTS=$+ $(COPTS) -dSMP
AOPTS=$+ $(AOPTS) -dSMP
!endif

CFLAGS=-ms -4 -bt=os2 -d2 -s -j -wx -zl -zfp -zgp -zq -zu -hc $(COPTS)

# Options for Watcom assembler
#  -bt=os2   = Build target OS is OS/2
#  -d1       = Include line number info in object
#              (necessary to produce assembler listing)
#  -i        = Include list
#  -zq       = Operate quietly
#  -3p       = 80386 protected-mode instructions
#
ASM=wasm
AFLAGS=-d1 -zq -3p $(AOPTS)

LINK=wlink
RC=rc
PPW = c:\ppwizard

#########################################
# Definitions for Help Compiler
#########################################
IPF=ipfc
L=ENU
P=437
C=1

# Inference rules
.c.obj: .AUTODEPEND
     $(CC) $(CFLAGS) $*.c
     wdis -l -s -e -p $*

.cpp.obj: .AUTODEPEND
     $(CPP) $(CFLAGS) $*.cpp
     wdis -l -s -e -p $*

.asm.obj: .AUTODEPEND
     $(ASM) $(AFLAGS) $*.asm
     wdis -l -s -e -p $*

# Object file list
OBJS1=segments.obj header.obj iprintf.obj mono.obj malloc.obj
OBJS2=strategy.obj init.obj ioctl.obj rmhelp.obj linklist.obj param.obj register.obj
OBJS3=isr.obj idc.obj mpu401.obj vdd.obj timer0.obj
OBJS4=stream.obj event.obj ssm_idc.obj midistrm.obj trace.obj
OBJS5=$(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4)
OBJS=$(OBJS5) build_id.obj

all: $(NAME).sys $(NAME).hlp $(NAME).inf html/$(NAME).htm cardinfo.dll $(NAME).dll mputrace.exe regstamp.exe

clean:
     del *.obj *.sys *.lst *.lrf *.exe *.def *.dll *.ipf *.hlp *.inf *.map html\*

build_id.obj: $(OBJS5) build_id.cmd
     build_id
     $(CC) $(CFLAGS) $*.c

mputrace.exe: mputrace.cpp .AUTODEPEND
   set INCLUDE=$(%WATCOM)\H;$(%WATCOM)\H\OS2
   wpp386 -s -j -oxsz -zm -5r -bm -bc -d2 -fpd -wx $*.cpp
   wlink format os2 flat file $*.obj option eliminate, debug, symfile=$*.dbg

regstamp.exe: regstamp.cpp .AUTODEPEND
   set INCLUDE=$(%WATCOM)\H;$(%WATCOM)\H\OS2
   wpp386 -s -j -oxsz -zm -5r -bm -bc -d2 -fpd -wx $*.cpp
   wlink format os2 flat file $*.obj option eliminate, debug, symfile=$*.dbg

$(NAME).lrf: build_id.obj
   @%append $^@ system os2 dll
   @%append $^@ option quiet
   @%append $^@ option verbose
   @%append $^@ option caseexact
   @%append $^@ option symfile=$(NAME).dbg
   @%append $^@ debug codeview
   @%append $^@ option cache
   @%append $^@ option map
   @%append $^@ name $(NAME).sys
   @for %f in ($(OBJS)) do @%append $^@ file %f
   @%append $^@ import DOSIODELAYCNT DOSCALLS.427
   @%append $^@ library ..\..\..\..\lib\os2286.lib
   @%append $^@ library ..\wpddlib\runtime.lib
   @%append $^@ library ..\..\resource\rmcalls\rmcalls.lib

$(NAME).sys: $(OBJS) $(NAME).lrf ..\..\..\..\lib\os2286.lib ..\wpddlib\runtime.lib ..\..\resource\rmcalls\rmcalls.lib
   $(LINK) @$(NAME).lrf

cardinfo.lrf: makefile
   @%write $^@ rcstub.obj
   @%write $^@ cardinfo.dll
   @%write $^@ cardinfo.map /batch /map /nod /noe /noi /packcode /packdata /exepack /align:16
   @%write $^@ os2286.lib
   @%write $^@ cardinfo.def;

cardinfo.def: makefile
   @%write $^@ LIBRARY cardinfo
   @%write $^@ DATA NONE

rcstub.obj: rcstub.c
   wcc386 -4 -bt=os2 -olinars -s -j -wx -zl -zq rcstub.c

cardinfo.dll: rcstub.obj cardinfo.rc cardinfo.lrf cardinfo.def
   link386 @cardinfo.lrf
   $(RC) cardinfo.rc cardinfo.dll

$(NAME).def: makefile
   @%write $^@ LIBRARY $(NAME)
   @%write $^@ DATA NONE

$(NAME).dll: $(NAME).rc $(NAME).def rcstub.obj
   link386 rcstub.obj, $(NAME).dll,resdll.map,,$(NAME).def
   $(RC) $(NAME).rc $(NAME).dll

$(NAME).ipf: $(NAME).it $(PPW)\ol_doc.dh
   $(PPW)\ppwizard /output:$(NAME).ipf $(NAME).it /crlf /define:DocType=IPF

html/$(NAME).htm: $(NAME).it $(PPW)\ol_doc.dh
   if exist html/* del /q /y /z html/*
   for %d in (*.bmp) do bmpgif2 %d
   $(PPW)\ppwizard /output:html\*.htm $(NAME).it /crlf /define:DocType=HTML
   move *.gif html

$(NAME).hlp: $(NAME).ipf
   $(IPF) /l=$(L) /codepage=$(P) /country=$(C) $(NAME).ipf $(NAME).hlp

$(NAME).inf: $(NAME).ipf
   $(IPF) /l=$(L) /codepage=$(P) /country=$(C) /i $(NAME).ipf $(NAME).inf

