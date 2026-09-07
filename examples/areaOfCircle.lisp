(defvar pi 3.1416)

(defun AreaOfCircle(radius)
    (let (area)
        (setq radius (* radius radius))
        (setq area (* pi radius))))

(print "Enter radius:")
(print (AreaOfCircle (read-integer)))