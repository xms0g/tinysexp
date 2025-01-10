[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	mov qword [rbp - 8], 11
	mov qword [rbp - 16], 12
	mov rax, qword [rbp - 8]
	mov rcx, qword [rbp - 16]
	add rax, rcx
	pop rbp
	ret