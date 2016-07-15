(println ((((fn foo [a b]
              (let [x 9]
                (fn [c d]
                  (fn [e f]
                    (+ a b x)))))
            1 2) 3 4) 5 6))

;; 12
