[bits 64]
section .text
	global _start
_start:
	push rbp
	mov rbp, rsp
	mov qword [rbp - 8], 0
.L0:
	mov rax, qword [rbp - 8]
	mov rcx, 3
	cmp rax, rcx
	jge .L1
	mov rax, qword [rel sum]
	mov rcx, qword [rbp - 8]
	add rax, rcx
	mov qword [rel sum], rax
	mov rax, qword [rbp - 8]
	add rax, 1
	mov qword [rbp - 8], rax
	jmp .L0
.L1:
	pop rbp
	ret

section .data
sum: dq 0

