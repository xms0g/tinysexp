(defun average (n)
    (let ((sum 0))
        (dotimes (i n)
            (setq sum (+ sum i)))
       (/ sum (- n 1))))

(print "Enter the number:")
(print (average (read-integer)))