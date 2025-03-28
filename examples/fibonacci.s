[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	mov rdi, 6
	call fibonacci
	mov r10, rax
	add rsp, 8
	pop rbp
	mov rax, 0x2000001
	xor rdi, rdi
	syscall

fibonacci:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	mov qword [rbp - 8], rdi
	mov r10, qword [rbp - 8]
	mov r11, 1
	cmp r10, r11
	jg .L0
	mov r10, qword [rel n]
	jmp .L2
.L0:
	mov r10, qword [rbp - 8]
	mov r11, 1
	sub r10, r11
	mov rdi, r10
	call fibonacci
	mov r10, rax
	mov r11, qword [rbp - 8]
	mov rdi, 2
	sub r11, rdi
	mov rdi, r11
	call fibonacci
	mov r11, rax
	add r10, r11
.L2:
	mov rax, r10
	add rsp, 8
	pop rbp
	ret
