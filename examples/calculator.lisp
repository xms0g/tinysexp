(defun calculator (n1 n2 op)
    (cond
        ((= op 1) (+ n1 n2))
        ((= op 2) (- n1 n2))
        ((= op 3) (* n1 n2))
        ((= op 4) (/ n1 n2))))

(defvar a)
(defvar b)
(defvar op)

(print "Enter the first number:")
(setq a (read-integer))
(print "Enter the second number:")
(setq b (read-integer))
(print "Enter the op:")
(setq op (read-integer))
(print (calculator a b op))