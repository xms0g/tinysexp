[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	movsd xmm0, qword [rel y]
	movsd qword [rel b], xmm0
	mov qword [rbp - 8], 11
	mov qword [rbp - 16], 3
	movsd xmm0, qword [rel b]
	movsd qword [rbp - 24], xmm0
	mov rax, qword [rbp - 8]
	movsd xmm0, qword [rbp - 24]
	mov rcx, qword [rbp - 16]
	cvtsi2sd xmm1, rcx
	mulsd xmm0, xmm1
	cvtsi2sd xmm1, rax
	addsd xmm1, xmm0
	movsd xmm0, xmm1
	pop rbp
	ret

section .data
y: dq 0x4037333340000000

section .bss
b: resq 1