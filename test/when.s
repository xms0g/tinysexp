[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	mov rax, qword [rel a]
	mov rcx, 4
	cmp rax, rcx
	jle .L0
	mov rax, qword [rel x]
	mov rcx, qword [rel y]
	add rax, rcx
	mov rcx, qword [rel a]
	mov rdx, 5
	cmp rcx, rdx
	jge .L1
	mov rcx, qword [rel x]
	mov rdx, qword [rel y]
	imul rcx, rdx
	jmp .L2
.L1:
	mov rdx, qword [rel x]
	mov rdi, qword [rel y]
	idiv rdx, rdi
.L2:
.L0:
	pop rbp
	ret

section .data
a: dq 5
y: dq 2
x: dq 3