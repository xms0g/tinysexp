(defvar pi 3.1416)

(defun AreaOfCircle()
    (let (radius area)
        (setq radius (read-integer))
        (setq radius (* radius radius))
        (setq area (* pi radius))
        (print area)))

(print "Enter radius:")
(AreaOfCircle)