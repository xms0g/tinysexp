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
	ret

calculator:
	push rbp
	mov rbp, rsp
	mov rax, 1
	cmp rdx, rax
	jne .L1
	add rdi, rsi
	jmp .L0
.L1:
	mov rax, 2
	cmp rdx, rax
	jne .L2
	sub rdi, rsi
	jmp .L0
.L2:
	mov rax, 3
	cmp rdx, rax
	jne .L3
	imul rdi, rsi
	jmp .L0
.L3:
	mov rax, 4
	cmp rdx, rax
	jne .L4
	mov rax, rdi
	cqo
	idiv rsi
	mov rdi, rax
	jmp .L0
.L4:
.L0:
	mov rax, rdi
	pop rbp
	ret

section .data
add-result: dq 0
sub-result: dq 0
mul-result: dq 0
div-result: dq 0
