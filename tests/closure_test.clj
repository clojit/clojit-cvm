(do (defn testfn [a]
      (fn [b]
        (+ a b)))
  (println ((testfn 10) 5)))
