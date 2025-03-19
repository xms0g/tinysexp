[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	mov rdi, 10
	call average
	mov r10, rax
	add rsp, 8
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
	mov r10, qword [rbp - 24]
	mov r11, qword [rbp - 8]
	cmp r10, r11
	jge .L1
	mov r10, qword [rbp - 16]
	mov r11, qword [rbp - 24]
	add r10, r11
	mov qword [rbp - 16], r10
	mov r10, qword [rbp - 24]
	add r10, 1
	mov qword [rbp - 24], r10
	jmp .L0
.L1:
	add rsp, 8
	mov r10, qword [rbp - 16]
	mov r11, qword [rbp - 8]
	mov rax, r10
	cqo
	idiv r11
	mov r10, rax
	add rsp, 8
	mov rax, r10
	add rsp, 8
	pop rbp
	ret
