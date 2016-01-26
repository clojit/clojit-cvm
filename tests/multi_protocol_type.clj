(do
       (defprotocol REST
         (GET [self])
         (POST [self a b])
         (PUT [self a b c]))

       (defprotocol REST2
         (PATCH [self]))

       (deftype API [x]
         REST
         (GET  [self] x)
         (POST [self a b] (+ a b))
         (PUT  [self a b c] (+ a b c))
         REST2
         (PATCH [self] 101))

      (deftype API2 [a b]
        REST2
        (PATCH [self] 101))

  (println (+
            (PATCH (->API2 5 5))       ;; 101
            (GET (->API 99))          ;; 99
            (PUT (->API 100) 10 20 20))) ;; 50

       )

;;Result 250
