(defvar y 23.2)
(defvar b)
(setq b y)

(let ((a 11) (x 3))
    (let ((c b))
        (+ a (* c x))
    )
)