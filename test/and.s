[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	movsd xmm0, qword [rel a]
	mov rax, 0
	cvtsi2sd xmm1, rax
	ucomisd xmm0, xmm1
	je .L0
	movsd xmm1, qword [rel b]
	mov rax, 0
	cvtsi2sd xmm2, rax
	ucomisd xmm1, xmm2
	je .L0
	movsd xmm0, qword [rel a]
	movsd xmm1, qword [rel b]
	addsd xmm0, xmm1
.L0:
	pop rbp
	ret

section .data
b: dq 0x4000000000000000
a: dq 0x3FF0000000000000
