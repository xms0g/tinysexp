extern _lrt_print_int
section .text
	global _main
_main:
	push rbp
	mov rbp, rsp
	mov rdi, 1
	mov rsi, 2
	mov rdx, 1
	call calculator
	mov r10, rax
	mov rdi, r10
	call _lrt_print_int
	mov r10, rax
	mov rdi, 3
	mov rsi, 2
	mov rdx, 2
	call calculator
	mov r11, rax
	mov rdi, r11
	call _lrt_print_int
	mov r11, rax
	mov rdi, 3
	mov rsi, 2
	mov rdx, 3
	call calculator
	mov rdi, rax
	mov rdi, rdi
	call _lrt_print_int
	mov rdi, rax
	mov rdi, 6
	mov rsi, 3
	mov rdx, 4
	call calculator
	mov rsi, rax
	mov rdi, rsi
	call _lrt_print_int
	mov rsi, rax
	xor rax, rax
	pop rbp
	ret

calculator:
	push rbp
	mov rbp, rsp
	sub rsp, 24
	mov qword [rbp - 8], rdi
	mov qword [rbp - 16], rsi
	mov qword [rbp - 24], rdx
	mov rdx, qword [rbp - 24]
	mov rcx, 1
	cmp rdx, rcx
	jne .L1
	mov rdx, qword [rbp - 8]
	mov rcx, qword [rbp - 16]
	add rdx, rcx
	jmp .L0
.L1:
	mov rdx, qword [rbp - 24]
	mov rcx, 2
	cmp rdx, rcx
	jne .L2
	mov rdx, qword [rbp - 8]
	mov rcx, qword [rbp - 16]
	sub rdx, rcx
	jmp .L0
.L2:
	mov rdx, qword [rbp - 24]
	mov rcx, 3
	cmp rdx, rcx
	jne .L3
	mov rdx, qword [rbp - 8]
	mov rcx, qword [rbp - 16]
	imul rdx, rcx
	jmp .L0
.L3:
	mov rdx, qword [rbp - 24]
	mov rcx, 4
	cmp rdx, rcx
	jne .L4
	mov rdx, qword [rbp - 8]
	mov rcx, qword [rbp - 16]
	mov rax, rdx
	cqo
	idiv rcx
	mov rdx, rax
	jmp .L0
.L4:
.L0:
	mov rax, rdx
	add rsp, 24
	pop rbp
	ret