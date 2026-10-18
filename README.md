![banner](banner.jpg)
# tinysexp
`tinysexp` is a minimalist Lisp compiler that targets the x86-64 architecture. It takes Lisp source code written in a simple s-expression syntax and compiles it down to NASM-compatible x86 assembly code following the System V ABI.

![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20MacOS-blue)
![License](https://img.shields.io/badge/license-GPL--3-green)
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
### Built-in Functions
`print`, `read-integer`, `read-double`, `read-string`

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
(print "Hello, World!")
```
**Output**
```asm
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
```
## License
This project is licensed under the GPL-3.0 License. See the LICENSE file for details.
