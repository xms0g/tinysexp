[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	mov rdi, 10
	call factorial
	mov r10, rax
	add rsp, 8
	pop rbp
	mov rax, 0x2000001
	xor rdi, rdi
	syscall

factorial:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	mov qword [rbp - 8], rdi
	mov r10, qword [rbp - 8]
	mov r11, 0
	cmp r10, r11
	jne .L0
	mov r10, 1
	jmp .L2
.L0:
	mov r10, qword [rbp - 8]
	mov r11, qword [rbp - 8]
	mov rdi, 1
	sub r11, rdi
	mov rdi, r11
	call factorial
	mov r11, rax
	imul r10, r11
.L2:
	mov rax, r10
	add rsp, 8
	pop rbp
	ret