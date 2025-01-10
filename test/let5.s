[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	mov rax, qword [rel x]
	mov rcx, qword [rel x]
	imul rax, rcx
	mov qword [rbp - 8], rax
	mov rax, qword [rbp - 8]
	mov rcx, qword [rbp - 8]
	add rax, rcx
	pop rbp
	ret

section .data
x: dq 3