(do
  (defprotocol IBar
    (total [self] [self a]))
  (deftype Bar [a]
    IBar
    (total [self] 5)
    (total [self a] (+ 9 a)))
  (println (total (->Bar 3))))
