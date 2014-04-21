;; Defines segment ordering for 16-bit DD's with MSC 6.0
;;
;; MODIFICATION HISTORY
;; DATE       PROGRAMMER   COMMENT
;; 01-Jul-95  Timur Tabi   Creation

.386
.seq

ifdef         DEBUG
SIGNATURE     equ       1
endif

_HEADER       segment dword public use16 'DATA'
_HEADER       ends

_DATA         segment dword public use16 'DATA'
_DATA         ends

CONST         segment dword public use16 'DATA'
CONST         ends

CONST2        segment dword public use16 'DATA'
CONST2        ends

_BSS          segment dword public use16 'BSS'
_BSS          ends

MEMBLOCK      struc
uSize         WORD      0
pmbNext       WORD      0
ifdef         SIGNATURE
ulSignature   DWORD     12345678h
endif
MEMBLOCK      ends

HEAPSIZE      equ       8192

; Having mbFirstFree and mbSecondFree insures that there is at least
; one free block.  This means that _pmbFree always points to a valid
; free block, and therefore never has to be NULL

_HEAP         segment dword public use16 'ENDDS'
public        _uMemFree
public        _pmbFree
public        _mbFirstFree
public        _end_of_heap
_uMemFree     WORD      HEAPSIZE
_pmbFree      WORD      offset _mbFirstFree
_mbFirstFree  MEMBLOCK  { HEAPSIZE, offset mbSecondFree }
              db        HEAPSIZE dup (0)
mbSecondFree  MEMBLOCK  { HEAPSIZE, 0 }
_end_of_heap  label     BYTE
_HEAP         ends

_ENDDS        segment dword public use16 'ENDDS'
public        _end_of_data
_end_of_data  db        0
_ENDDS        ends

_INITDATA     segment dword public use16 'ENDDS'
_INITDATA     ends

_TEXT         segment dword public use16 'CODE'
_TEXT         ends

_ENDCS        segment dword public use16 'CODE'
public        end_of_text_
end_of_text_  proc
end_of_text_  endp
_ENDCS        ends

RMCODE        SEGMENT DWORD PUBLIC USE16 'CODE'
RMCODE        ENDS

_INITTEXT     segment dword public use16 'CODE'
_INITTEXT     ends

DGROUP        group _HEADER, CONST, CONST2, _DATA, _BSS, _HEAP, _ENDDS, _INITDATA
CGROUP        GROUP _TEXT, _ENDCS, RMCODE, _INITTEXT


_TEXT         segment dword public use16 'CODE'

; Route VDD IDC request to C code to implement
; 16:16 entry from 16:32 caller
;
; Called from VDD using pascal calling conventions:
;       Parms pushed left to right, callee clears the stack
;
ulFunc  equ dword ptr [bp+18]   ; Pascal conventions push parms left to right
ul1     equ dword ptr [bp+14]   ; 8 bytes for 16:32 far return address
ul2     equ dword ptr [bp+10]   ; 2 bytes for save of caller's stack frame ptr

                public  _PDDEntryPoint
                extrn   _PDDEntryPoint2 : near

spacer          proc    far
                ret
spacer          endp

_PDDEntryPoint  proc    far                     ; 16:16 entry from 16:32
                push    bp
                mov     bp, sp

                push    ds
                push    es
                push    ebx
                push    ecx
                push    edx
                push    esi
                push    edi

                mov     ax, seg _DATA
                mov     ds, ax
                push    ul2
                push    ul1
                push    ulFunc                  ; cdecl calling convention
                call    _PDDEntryPoint2
                add     esp,12
                shl     edx, 16                 ; Move DX:AX return value
                mov     dx, ax                  ; into eax
                mov     eax, edx

                pop     edi                     ; Restore callers regs
                pop     esi
                pop     edx
                pop     ecx
                pop     ebx
                pop     es
                pop     ds

                pop     bp
                db      66h                     ; Force next instruction 32-bit
                ret     12                      ; 16:32 far return, pop parms
_PDDEntryPoint  endp

_TEXT           ends

end

