extern _lrt_print_int
extern _lrt_print_double
extern _lrt_print_str
section .text
	global _main
_main:
	push rbp
	mov rbp, rsp
	mov rdi, 10
	call average
	mov r10, rax
	mov rdi, r10
	call _lrt_print_int
	mov r10, rax
	xor eax, eax
	pop rbp
	ret

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
	mov rdi, 1
	sub r11, rdi
	mov rax, r10
	cqo
	idiv r11
	mov r10, rax
	add rsp, 8
	mov rax, r10
	add rsp, 8
	pop rbp
	ret
