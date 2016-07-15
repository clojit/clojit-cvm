(println (((fn [a]
             (fn [b]
               (let [fnc (fn [c] (+ c a))
                     fnd (fn [d]  (+ d a))
                     fne (fn [e] (+ e a))]
                 (fnc (fnd (fne (+ a b))))))) 5) 5))

;; 25

