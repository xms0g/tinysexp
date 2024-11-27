# lbf
**lbf** is a lisp to brainfuck compiler, enabling you to write programs in a Lisp-like language and compile them into the minimalist Brainfuck programming language.
## Features
A subset of Lisp, including:
- `Arithmetic operations:` Basic operations (`+`, `-`, `*`, `/`).
- `Conditionals:` Basic branching with `if`.
- `Looping:` Iterates with `dotimes`.
<!--
- Functions: Define reusable functions using defun.
-->
- `Variables:` Simple variable assignments and scopes with `let` and `setq`.
- `Input/Output:` Support for `print` and `read`.

## Calling Conventions
- Result value is always in the first cell.
## Usage

## Example
```lisp
(* 3 4)
(dotimes (i 5) (+ 3 4))
(print (* 2 (+ 3 (/ 9 3))))
(let ((a 3) (b 4) (print (+ a b)))
(let (a) (setq a 10))

```

## License

## Credits
