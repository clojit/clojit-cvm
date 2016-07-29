(let [	z 1
	y (fn []  (let [a 1
			b 2
			c 3
			d 4
			e 5]
		((fn [a b c d e] (+ a (+ b (+ c (+ d e))))) a b c d e)))]
	(y 81)
	((fn [y] (+ y 1)) 47))
