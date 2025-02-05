# tinysexp
`tinysexp` is a lisp  compiler
## Features
A subset of Lisp, including:
### Operators
- Arithmetic Operators: `+`, `-`, `*`, `/`.
- Comparison Operators: `=`, `/=`, `>`, `<`, `>=`, `<=`.
- Logical Operators: `and`, `or`, `not`
### Condition
- `if`,`when` and `cond`
### Loop
- `dotimes` and `loop`
### Functions
- `defun`
### Variables
- Local Variables: with `let` and `setq`.
- Global Variables: with `defvar`.
- Constant Variables: with `defconstant`.
## ABI
## Usage

## Example
```scheme
(* 3 4)
(dotimes (i 5) (+ 3 4))
(print (* 2 (+ 3 (/ 9 3))))
(let ((a 3) (b 4) (print (+ a b)))
(let (a) (setq a 10))

```

## License
