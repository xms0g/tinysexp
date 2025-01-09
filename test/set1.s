[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	movsd xmm0, qword [rel x]
	movsd xmm1, qword [rel x]
	mulsd xmm0, xmm1
	movsd qword [rel xsqr], xmm0
	mov rax, qword [rel y]
	mov rcx, qword [rel y]
	imul rax, rcx
	mov qword [rel ysqr], rax
	movsd xmm0, qword [rel xsqr]
	mov rax, qword [rel ysqr]
	cvtsi2sd xmm1, rax
	addsd xmm0, xmm1
	movsd qword [rel z], xmm0
	pop rbp
	ret

section .data
y: dq 4
x: dq 0x40099999A0000000

section .bss
z: resq 1
ysqr: resq 1
xsqr: resq 1