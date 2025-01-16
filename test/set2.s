[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	mov rax, 2
	mov rcx, 2
	cmp rax, rcx
	setl al
	movzx rax, al
	mov qword [rel x], rax
	pop rbp
	ret

section .bss
x: resq 1
