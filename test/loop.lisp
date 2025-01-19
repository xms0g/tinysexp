(defvar a)
(setq a 0)

(loop
    (setq a (+ a 1))
    (+ a a)
    (when (> a 10) (return a))
)


