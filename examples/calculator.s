[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	mov rdi, 1
	mov rsi, 2
	mov rdx, 1
	call calculator
	mov r10, rax
	add rsp, 8
	mov qword [rel add-result], r10
	sub rsp, 8
	mov rdi, 3
	mov rsi, 2
	mov rdx, 2
	call calculator
	mov r10, rax
	add rsp, 8
	mov qword [rel sub-result], r10
	sub rsp, 8
	mov rdi, 3
	mov rsi, 2
	mov rdx, 3
	call calculator
	mov r10, rax
	add rsp, 8
	mov qword [rel mul-result], r10
	sub rsp, 8
	mov rdi, 4
	mov rsi, 2
	mov rdx, 4
	call calculator
	mov r10, rax
	add rsp, 8
	mov qword [rel div-result], r10
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
	mov r10, qword [rbp - 24]
	mov r11, 1
	cmp r10, r11
	jne .L1
	mov r10, qword [rbp - 8]
	mov r11, qword [rbp - 16]
	add r10, r11
	jmp .L0
.L1:
	mov r10, qword [rbp - 24]
	mov r11, 2
	cmp r10, r11
	jne .L2
	mov r10, qword [rbp - 8]
	mov r11, qword [rbp - 16]
	sub r10, r11
	jmp .L0
.L2:
	mov r10, qword [rbp - 24]
	mov r11, 3
	cmp r10, r11
	jne .L3
	mov r10, qword [rbp - 8]
	mov r11, qword [rbp - 16]
	imul r10, r11
	jmp .L0
.L3:
	mov r10, qword [rbp - 24]
	mov r11, 4
	cmp r10, r11
	jne .L4
	mov r10, qword [rbp - 8]
	mov r11, qword [rbp - 16]
	mov rax, r10
	cqo
	idiv r11
	mov r10, rax
	jmp .L0
.L4:
.L0:
	mov rax, r10
	add rsp, 24
	pop rbp
	ret

section .data
add-result: dq 0
sub-result: dq 0
mul-result: dq 0
div-result: dq 0
