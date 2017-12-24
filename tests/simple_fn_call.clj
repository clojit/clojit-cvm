(println (do
  (def a "abc")

 ((fn [a] 
   (+ a 10)) 5)))
