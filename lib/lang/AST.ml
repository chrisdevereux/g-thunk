(* Main structures *)

type module_node = {module_name: string; module_bindings: binding_node list}

and binding_node = {binding_name: string; binding_value: expr}

and expr =
  | Literal of literal_node
  | Fn of fn_node
  | Call of call_node
  | Ref of ref_node

and literal_node =
  | Nothing
  | Int of int

and fn_node = {fn_params: string list; fn_bindings: binding_node list; fn_value: expr}

and call_node = {operator: expr; operand: expr}

and ref_node = {ref_label: string}

let bound_expressions = List.map (fun x -> x.binding_value)

let expr_children (e: expr): expr list =
  match e with
    | Literal _ -> []
    | Fn {fn_bindings; fn_value} -> fn_value :: bound_expressions fn_bindings
    | Call {operator; operand} -> [operator; operand]
    | Ref _ -> []

  (*
  *)
