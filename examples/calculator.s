[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	sub rsp, 16
	mov rdi, 1
	mov rsi, 2
	mov rdx, 1
	call calculator
	add rsp, 16
	mov qword [rel add-result], rax
	sub rsp, 16
	mov rdi, 3
	mov rsi, 2
	mov rdx, 2
	call calculator
	add rsp, 16
	mov qword [rel sub-result], rax
	sub rsp, 16
	mov rdi, 3
	mov rsi, 2
	mov rdx, 3
	call calculator
	add rsp, 16
	mov qword [rel mul-result], rax
	sub rsp, 16
	mov rdi, 4
	mov rsi, 2
	mov rdx, 4
	call calculator
	add rsp, 16
	mov qword [rel div-result], rax
	pop rbp
	mov rax, 0x2000001
	xor rdi, rdi
	syscall

calculator:
	push rbp
	mov rbp, rsp
	sub rsp, 24
	mov qword [rbp - 8], rdi
	mov qword [rbp - 16], rsi
	mov qword [rbp - 24], rdx
	mov rax, qword [rbp - 24]
	mov r10, 1
	cmp rax, r10
	jne .L1
	mov rax, qword [rbp - 8]
	mov r10, qword [rbp - 16]
	add rax, r10
	jmp .L0
.L1:
	mov rax, qword [rbp - 24]
	mov r10, 2
	cmp rax, r10
	jne .L2
	mov rax, qword [rbp - 8]
	mov r10, qword [rbp - 16]
	sub rax, r10
	jmp .L0
.L2:
	mov rax, qword [rbp - 24]
	mov r10, 3
	cmp rax, r10
	jne .L3
	mov rax, qword [rbp - 8]
	mov r10, qword [rbp - 16]
	imul rax, r10
	jmp .L0
.L3:
	mov rax, qword [rbp - 24]
	mov r10, 4
	cmp rax, r10
	jne .L4
	mov rax, qword [rbp - 8]
	mov r10, qword [rbp - 16]
	cqo
	idiv r10
	jmp .L0
.L4:
.L0:
	pop rbp
	add rsp, 24
	ret

section .data
add-result: dq 0
sub-result: dq 0
mul-result: dq 0
div-result: dq 0