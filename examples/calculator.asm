extern _lrt_print_int
extern _lrt_print_double
extern _lrt_print_str
extern _lrt_read_int
extern _lrt_read_double
extern _lrt_read_str
section .text
	global _main
_main:
	push rbp
	mov rbp, rsp
	lea rdi, [rel str.0]
	call _lrt_print_str
	mov r10, rax
	call _lrt_read_int
	mov r10, rax
	mov qword [rel a], r10
	lea rdi, [rel str.1]
	call _lrt_print_str
	mov r10, rax
	call _lrt_read_int
	mov r10, rax
	mov qword [rel b], r10
	lea rdi, [rel str.2]
	call _lrt_print_str
	mov r10, rax
	call _lrt_read_int
	mov r10, rax
	mov qword [rel op], r10
	mov rdi, qword [rel a]
	mov rsi, qword [rel b]
	mov rdx, qword [rel op]
	call calculator
	mov r10, rax
	mov rdi, r10
	call _lrt_print_int
	mov r10, rax
	xor eax, eax
	leave
	ret

calculator:
	push rbp
	mov rbp, rsp
	sub rsp, 24
	mov qword [rbp - 8], rdi
	mov qword [rbp - 16], rsi
	mov qword [rbp - 24], rdx
	mov r10, qword [rel op]
	mov r11, 1
	cmp r10, r11
	jne .L1
	mov r10, qword [rbp - 8]
	mov r11, qword [rbp - 16]
	add r10, r11
	jmp .L0
.L1:
	mov r10, qword [rel op]
	mov r11, 2
	cmp r10, r11
	jne .L2
	mov r10, qword [rbp - 8]
	mov r11, qword [rbp - 16]
	sub r10, r11
	jmp .L0
.L2:
	mov r10, qword [rel op]
	mov r11, 3
	cmp r10, r11
	jne .L3
	mov r10, qword [rbp - 8]
	mov r11, qword [rbp - 16]
	imul r10, r11
	jmp .L0
.L3:
	mov r10, qword [rel op]
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
	leave
	ret

section .rodata
str.0: db "Enter the first number:", 10, 0
str.1: db "Enter the second number:", 10, 0
str.2: db "Enter the op:", 10, 0

section .bss
a: resq 1
b: resq 1
op: resq 1
