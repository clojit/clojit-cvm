(do
  (defprotocol IBar
    (total [self]
           [self b]))

  (defprotocol INoArg
    (stuff [self]))

  (deftype Bar [a]
    IBar
    (total [self] "Bar total 1 arg")                                      ; 12820
    (total [self b] :Bar-total-2-arg)
    INoArg
    (stuff [self] 5556464))

  (deftype Foo [c]
    IBar
    (total [self] "Foo total 1 arg")
    (total [self b] :Foo-total-2-arg)
    INoArg
    (stuff [self] 888.88))

  (total (->Bar 1)))