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
	xor eax, eax
	leave
	ret

section .rodata
str.0:
	db "Hello, World!", 10, 0
