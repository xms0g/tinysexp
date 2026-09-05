# tinysexp
`tinysexp` is a minimalist Lisp compiler that targets the x86-64 architecture. It takes Lisp source code written in a simple s-expression syntax and compiles it down to NASM-compatible x86 assembly code.

## Features
A subset of Lisp, including:
### Arithmetic Operations
`+`,`-`,`*`,`/`
### Comparison Operations
`=`,`/=`,`>`,`<`,`>=`,`<=`
### Logical Operations
`and`,`or`,`not`
### Bitwise Operations
`logand`,`logior`,`logxor`,`lognor`
### Conditionals
`if`,`when`,`cond`
### Loop
`dotimes`,`loop`
### Functions
`defun`
### Variables
`let`,`setq`,`defvar`,`defconstant`

## Usage
```
➜  ~ tinysexp -h
OVERVIEW: Lisp compiler for x86-64 architecture

USAGE: tinysexp [options] file

OPTIONS:
  -o, --output          The output file name
  -h, --help            Display available options
  -v, --version         Display the version of this program
```
## Example
**Input**
```lisp
(defun add (a b)
  (+ a b))

(print (add 1 2))
```
**Output**
```asm
extern _lrt_print_int
extern _lrt_print_double
extern _lrt_print_str
section .text
    global _main
_main:
    push rbp
    mov rbp, rsp
    mov rdi, 1
    mov rsi, 2
    call add
    mov r10, rax
    mov rdi, r10
    call _lrt_print_int
    mov r10, rax
    xor rax, rax
    pop rbp
    ret

add:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov qword [rbp - 8], rdi
    mov qword [rbp - 16], rsi
    mov r10, qword [rbp - 8]
    mov r11, qword [rbp - 16]
    add r10, r11
    mov rax, r10
    add rsp, 16
    pop rbp
    ret
```
## License
This project is licensed under the GPL-3.0 License. See the LICENSE file for details.
