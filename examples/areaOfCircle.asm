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
	call AreaOfCircle
	mov r10, rax
	xor eax, eax
	leave
	ret

AreaOfCircle:
	push rbp
	mov rbp, rsp
	sub rsp, 16
	call _lrt_read_int
	mov r10, rax
	mov qword [rbp - 8], r10
	mov r10, qword [rbp - 8]
	mov r11, qword [rbp - 8]
	imul r10, r11
	mov qword [rbp - 8], r10
	movsd xmm1, qword [rel pi]
	mov r10, qword [rbp - 8]
	cvtsi2sd xmm2, r10
	mulsd xmm1, xmm2
	movsd qword [rbp - 16], xmm1
	movsd xmm0, qword [rbp - 16]
	call _lrt_print_double
	movsd xmm1, xmm0
	add rsp, 16
	leave
	ret

section .rodata
str.0: db "Enter radius:", 10, 0

section .data
pi: dq 0x400921FF20000000
