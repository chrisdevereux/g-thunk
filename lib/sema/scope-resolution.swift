class ScopeContext<T> {
  lazy var scopes = Dictionary<String, T>()
  let parent: ScopeContext<T>?
  
  init(parentScope: ScopeContext<T>? = nil) {
    parent = parentScope
  }
  
  func resolve(id: String) -> T? {
    return scopes[id] ?? parent?.resolve(id)
  }
  
  func next() -> ScopeContext<T> {
    return ScopeContext(parentScope: self)
  }
}
