[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	mov rax, qword [rel d]
	mov rcx, 5
	cmp rax, rcx
	jge .L1
	mov rax, qword [rel x]
	mov rcx, qword [rel y]
	add rax, rcx
	jmp .L0
.L1:
	mov rcx, qword [rel d]
	mov rdx, 5
	cmp rcx, rdx
	jne .L2
	mov rcx, qword [rel x]
	mov rdx, qword [rel y]
	imul rcx, rdx
	jmp .L0
.L2:
	mov rdx, qword [rel d]
	mov rdi, 5
	cmp rdx, rdi
	jle .L3
	mov rdx, qword [rel x]
	mov rdi, qword [rel y]
	idiv rdx, rdi
	jmp .L0
.L3:
.L0:
	pop rbp
	ret

section .data
d: dq 2
y: dq 2
x: dq 3