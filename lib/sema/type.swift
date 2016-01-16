/** Types **/

indirect enum AbstractType {
  case Primitive(PrimitiveType)
  case Union(Set<AbstractType>)
  case Intersection(Set<AbstractType>)
  case Field(String, AbstractType)
  case List([AbstractType])
  case Function(AbstractType, AbstractType)
}

indirect enum ConcreteType {
  case Primitive(PrimitiveType)
  case Struct([(String, ConcreteType)])
  case Array([ConcreteType])
}

enum PrimitiveType {
  case F32
}


/** Hashable **/

extension AbstractType : Hashable {
  var hashValue: Int {
    get {
      switch self {
      case .Primitive(let t): return t.hashValue
      case .Union(let types): return types.hashValue
      case .Intersection(let types): return types.hashValue
      case .Field(let key, let t): return key.hashValue | t.hashValue
      case .List(let types): return hashArray(types, hashFn: defaultHash)
      case .Function(let param, let val): return param.hashValue | val.hashValue
      }
    }
  }
}

extension ConcreteType : Hashable {
  var hashValue: Int {
    get {
      switch self {
      case .Primitive(let t): return t.hashValue
      case .Struct(let types): return hashArray(types, hashFn: pairHash)
      case .Array(let types): return hashArray(types, hashFn: defaultHash)
      }
    }
  }
}

func hashArray<T>(array: [T], hashFn: (T) -> Int) -> Int {
  return array.reduce(0){
    (hash, x) in return hash | hashFn(x)
  }
}

func defaultHash<T: Hashable>(x: T) -> Int {
  return x.hashValue
}

func pairHash<L: Hashable, R: Hashable>(x: (L, R)) -> Int {
  let (l, r) = x
  return l.hashValue | r.hashValue
}


/** Equatable **/

extension AbstractType : Equatable {}
func == (lhs: AbstractType, rhs: AbstractType) -> Bool {
  switch (lhs, rhs) {
  case (.Primitive(let l), .Primitive(let r)): return l.hashValue == r.hashValue
  case (.Union(let l), .Union(let r)): return l == r
  case (.Intersection(let l), .Union(let r)): return l == r
  case (.Field(let ll, let lr), .Field(let rl, let rr)): return ll == rl && lr == rr
  case (.List(let l), .List(let r)): return equalsArray(l, with: r, equalsFn: defaultEquals)
  case (.Function(let ll, let lr), .Function(let rl, let rr)): return ll == rl && lr == rr
  }
}


extension ConcreteType : Equatable {
  
}

func equalsArray<T>(lArray: [T], with: [T], equalsFn: (T, T) -> Bool) -> Bool {
  for (l, r) in zip(lArray, with) {
    if !equalsFn(l, r) {
      return false
    }
  }
  
  return true
}

func defaultEquals<T: Equatable>(l: T, r: T) -> Bool {
  return l == r
}

func pairEquals<T: Equatable>(l: T, r: T) -> Bool {
  return l == r
}
