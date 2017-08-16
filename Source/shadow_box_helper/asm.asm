;
;                       Shadow-Box Helper
;                       -----------------
;      Lightweight Hypervisor-Based Kernel Protector
;
;               Copyright (C) 2017 Seunghun Han
;     at National Security Research Institute of South Korea
;

; This software has dual license (MIT and GPL v2). See the GPL_LICENSE and
; MIT_LICENSE file.

[bits 64]
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Exported functions.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
global sbh_vm_call

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Imported functions.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Call vmcall.
;	VMcall argument:
;		rax: service number
;		rbx: argument
;	Result:
;		rax: return value
sbh_vm_call:
	push rbx

	mov rax, rdi
	mov rbx, rsi

	vmcall

	pop rbx
	ret
