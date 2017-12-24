(println (loop [x 0 y 1 z 2]
           (if (== x 10)
             x
             (recur (+ x 1) (+ 19 10 10 1 (+ y 1)) (+ y (+ z 1))))))

;; Result 10
