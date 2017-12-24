(do
  (deftype TestT [a b c])
  (println (. (->TestT 6) b)))