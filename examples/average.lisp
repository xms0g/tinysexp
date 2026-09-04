(defun average (n)
    (let ((sum 0))
        (dotimes (i n)
            (setq sum (+ sum i)))
       (/ sum n)))

(print (average 10))