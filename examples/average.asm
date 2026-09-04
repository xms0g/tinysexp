section .text
	global _main
_main:
	mov rdi, 10
	call average
	mov r10, rax
	mov rdi, r10
	call print_int
	mov r10, rax
	xor rax, rax
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
	mov r11, qword [rbp - 24]
	mov rdi, qword [rbp - 8]
	cmp r11, rdi
	jge .L1
	mov r11, qword [rbp - 16]
	mov rdi, qword [rbp - 24]
	add r11, rdi
	mov qword [rbp - 16], r11
	mov r11, qword [rbp - 24]
	add r11, 1
	mov qword [rbp - 24], r11
	jmp .L0
.L1:
	add rsp, 8
	mov r11, qword [rbp - 16]
	mov rdi, qword [rbp - 8]
	mov rax, r11
	cqo
	idiv rdi
	mov r11, rax
	add rsp, 8
	mov rax, r11
	add rsp, 8
	pop rbp
	ret

print_int:
	push rbp
	mov rbp, rsp
	sub rsp, 64
	mov rax, rdi
	mov [rbp - 64], rdi
	lea rsi, [rbp - 1]
	mov byte [rsi], 10
	mov rcx, 1
	test rax, rax
	jne .convert
	dec rsi
	mov byte [rsi], '0'
	inc rcx
	jmp .write
.convert:
	mov r8, 10
.loop:
	xor rdx, rdx
	div r8
	add dl, '0'
	dec rsi
	mov [rsi], dl
	inc rcx
	test rax, rax
	jne .loop
.write:
	mov rax, 0x2000004
	mov rdi, 1
	mov rdx, rcx
	syscall
	mov rax, [rbp - 64]
	leave
	ret
