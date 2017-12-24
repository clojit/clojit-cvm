(do
  (defprotocol IBar
    (total [self]
      [self a]))
  (deftype Bar [a b c]
    IBar
    (total [self] 993)                                      ; 12820
    (total [self a] (+ 993 a)))                             ; 12821

  (println (total (->Bar 1))))
