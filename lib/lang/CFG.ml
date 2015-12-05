module IDMap = Map.Make(String);;


(* Value types *)

type value =
  | Float of float

type val_type =
  | FloatT


(* Basic Block CFG Graph *)

type ssa_node =
  | Case of cases
  | Call of fn_call
  | Ref of string
  | Const of value
  | Param
(** CFG logic node *)

and cases = {condition: ssa_node; cases: case_cond list}
and case_cond = {case_cond: value; case_result: ssa_node}
and fn_call = {call_proc: string; call_param: ssa_node}


(* Procedure & Program Structure *)

type block = {block_id: string; value: ssa_ snode}
(**
  Roots a SSA graph.
  Represents single register/stack assignment or procedure return value
*)

type proc = {proc_blocks: ssa_node IDMap.t; proc_main: string}
(** Represents a procedure *)

type program = {prog_procs: proc IDMap.t; prog_start: string}
(** Represents a program *)

(*
*)
