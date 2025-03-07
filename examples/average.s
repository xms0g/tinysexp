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
	mov rax, 0x2000001
	xor rdi, rdi
	syscall

average:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	mov qword [rbp - 8], rdi
	sub rsp, 8
	mov qword [rbp - 16], 0
	sub rsp, 8
	mov qword [rbp - 24], 0
.L0:
	mov rax, qword [rbp - 24]
	mov r10, qword [rbp - 8]
	cmp rax, r10
	jge .L1
	mov rax, qword [rbp - 16]
	mov r10, qword [rbp - 24]
	add rax, r10
	mov qword [rbp - 16], rax
	mov rax, qword [rbp - 24]
	add rax, 1
	mov qword [rbp - 24], rax
	jmp .L0
.L1:
	add rsp, 8
	mov rax, qword [rbp - 16]
	mov r10, qword [rbp - 8]
	cqo
	idiv r10
	add rsp, 8
	pop rbp
	add rsp, 8
	ret