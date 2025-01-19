[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	mov qword [rel a], 0
.L0:
	mov rax, qword [rel a]
	mov rcx, 1
	add rax, rcx
	mov qword [rel a], rax
	mov rax, qword [rel a]
	mov rcx, qword [rel a]
	add rax, rcx
	mov rcx, qword [rel a]
	mov rdx, 10
	cmp rcx, rdx
	jle .L0
	jmp .L1
.L1:
	pop rbp
	ret

section .bss
a: resq 1