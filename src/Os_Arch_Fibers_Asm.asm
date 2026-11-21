; OSEKOS Implementation of an OSEK Scheduler
; Copyright (C) 2015 Joakim Plate
;
; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Lesser General Public
; License as published by the Free Software Foundation; either
; version 2.1 of the License, or (at your option) any later version.

; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; Lesser General Public License for more details.

; You should have received a copy of the GNU Lesser General Public
; License along with this library; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    PUBLIC Os_Arch_Trampoline
IFDEF RAX
ELSE
    .MODEL flat, c
ENDIF
    .CODE

Os_Arch_Trampoline PROC
IFDEF RAX
    ; The interrupted stack pointer may have any alignment, and the
    ; x64 calling convention requires a 16 byte aligned stack and 32
    ; bytes of shadow space owned by the callee at the call.
    push rbp
    mov  rbp, rsp
    and  rsp, -16
    sub  rsp, 20h
    call rdx
    mov  rsp, rbp
    pop  rbp
    pop rax
    pop rcx
    pop rdx
    pop r8
    pop r9
    pop r10
    pop r11
    ret
ELSE
    call ecx
    add esp, 4  ;clean stack from parameters
    pop eax
    pop ecx
    pop edx
    pop ebp
    ret
ENDIF
Os_Arch_Trampoline ENDP

    END
