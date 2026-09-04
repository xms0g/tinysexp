(defun calculator (n1 n2 op)
    (cond
        ((= op 1) (+ n1 n2))
        ((= op 2) (- n1 n2))
        ((= op 3) (* n1 n2))
        ((= op 4) (/ n1 n2))))

(print (calculator 1 2 1))
(print (calculator 3 2 2))
(print (calculator 3 2 3))
(print (calculator 6 3 4))
