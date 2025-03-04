(defvar add-result 0)
(defvar sub-result 0)
(defvar mul-result 0)
(defvar div-result 0)

(defun calculator (n1 n2 op)
    (cond
        ((= op 1) (+ n1 n2))
        ((= op 2) (- n1 n2))
        ((= op 3) (* n1 n2))
        ((= op 4) (/ n1 n2))
    ))

(setq add-result (calculator 1 2 1))
(setq sub-result (calculator 3 2 2))
(setq mul-result (calculator 3 2 3))
(setq div-result (calculator 4 2 4))
