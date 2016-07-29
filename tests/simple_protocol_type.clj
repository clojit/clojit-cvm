(do
    (defprotocol REST
        (GET [self]))
    (deftype API [a]
        REST
        (GET [self] a))
  (println (GET (->API 1001)))
)

