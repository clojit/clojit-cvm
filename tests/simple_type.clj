(do
  (deftype TestT [a b c])
  (. (->TestT 6) b))