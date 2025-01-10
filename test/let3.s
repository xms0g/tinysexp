[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	mov qword [rbp - 8], 11
	mov qword [rbp - 24], 3
	mov qword [rbp - 16], 2
	mov qword [rbp - 32], 4
	mov rax, qword [rbp - 8]
	mov rcx, qword [rbp - 16]
	mov rdx, qword [rbp - 32]
	mov rdi, qword [rbp - 24]
	sub rdx, rdi
	imul rcx, rdx
	add rax, rcx
	pop rbp
	ret
