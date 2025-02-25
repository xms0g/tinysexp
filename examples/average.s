[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	sub rsp, 16
	mov rdi, 10
	call average
	add rsp, 16
	pop rbp
	ret

average:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	mov qword [rbp - 8], 0
	sub rsp, 8
	mov qword [rbp - 16], 0
.L0:
	mov rax, qword [rbp - 16]
	cmp rax, rdi
	jge .L1
	mov rax, qword [rbp - 8]
	mov r10, qword [rbp - 16]
	add rax, r10
	mov qword [rbp - 8], rax
	mov rax, qword [rbp - 16]
	add rax, 1
	mov qword [rbp - 16], rax
	jmp .L0
.L1:
	add rsp, 8
	mov rax, qword [rbp - 8]
	cqo
	idiv rdi
	add rsp, 8
	pop rbp
	ret