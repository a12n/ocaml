(***********************************************************************)
(*                                                                     *)
(*                                OCaml                                *)
(*                                                                     *)
(*                  Fabrice Le Fessant, INRIA Saclay                   *)
(*                                                                     *)
(*  Copyright 2012 Institut National de Recherche en Informatique et   *)
(*  en Automatique.  All rights reserved.  This file is distributed    *)
(*  under the terms of the Q Public License version 1.0.               *)
(*                                                                     *)
(***********************************************************************)

open Cmi_format
open Typedtree

(* Note that in Typerex, there is an awful hack to save a cmt file
   together with the interface file that was generated by ocaml (this
   is because the installed version of ocaml might differ from the one
   integrated in Typerex).
*)

let read_magic_number ic =
  let len_magic_number = String.length Config.cmt_magic_number in
  let magic_number = String.create len_magic_number in
  really_input ic magic_number 0 len_magic_number;
  magic_number

type binary_annots =
  | Packed of Types.signature * string list
  | Implementation of structure
  | Interface of signature
  | Partial_implementation of binary_part array
  | Partial_interface of binary_part array

and binary_part =
| Partial_structure of structure
| Partial_structure_item of structure_item
| Partial_expression of expression
| Partial_pattern of pattern
| Partial_class_expr of class_expr
| Partial_signature of signature
| Partial_signature_item of signature_item
| Partial_module_type of module_type

type cmt_infos = {
  cmt_modname : string;
  cmt_annots : binary_annots;
  cmt_comments : (string * Location.t) list;
  cmt_args : string array;
  cmt_sourcefile : string;
  cmt_builddir : string;
  cmt_loadpath : string list;
  cmt_packed : string list;
  cmt_source_digest : string option;
  cmt_initial_env : Env.t;
(* TODO
  cmt_crcs : (string * Digest.t) list;
  cmt_flags : Env.pers_flags list;
*)
}

type error =
    Not_a_typedtree of string

exception Error of error

let input_cmt ic = (input_value ic : cmt_infos)

let output_cmt oc cmt =
  output_string oc Config.cmt_magic_number;
  output_value oc (cmt : cmt_infos)

let read filename =
(*  Printf.fprintf stderr "Cmt_format.read %s\n%!" filename; *)
  let ic = open_in filename in
  try
    let magic_number = read_magic_number ic in
    let cmi, cmt =
      if magic_number = Config.cmt_magic_number then
        None, Some (input_cmt ic)
      else if magic_number = Config.cmi_magic_number then
        let cmi = Cmi_format.input_cmi ic in
        let cmt = try
                    let magic_number = read_magic_number ic in
                    if magic_number = Config.cmt_magic_number then
                      let cmt = input_cmt ic in
                      Some cmt
                    else None
          with _ -> None
        in
        Some cmi, cmt
      else
        raise(Cmi_format.Error(Cmi_format.Not_an_interface filename))
    in
    close_in ic;
(*    Printf.fprintf stderr "Cmt_format.read done\n%!"; *)
    cmi, cmt
  with e ->
    close_in ic;
    raise e

let string_of_file filename =
  let ic = open_in filename in
  let s = Misc.string_of_file ic in
  close_in ic;
  s

let read_cmt filename =
  match read filename with
      _, None -> raise (Error (Not_a_typedtree filename))
    | _, Some cmt -> cmt

let read_cmi filename =
  match read filename with
      None, _ -> raise (Cmi_format.Error (Cmi_format.Not_an_interface filename))
    | Some cmi, _ -> cmi

let saved_types = ref []

let add_saved_type b = saved_types := b :: !saved_types
let get_saved_types () = !saved_types
let set_saved_types l = saved_types := l

let save_cmt modname filename binary_annots sourcefile packed_modules initial_env sg =
  if !Clflags.binary_annotations && not !Clflags.print_types then begin
    let oc = open_out filename in
    begin
      match sg with
          None -> ()
        | Some (sg, imports) ->
          let cmi =
                {
                  cmi_name = modname;
                  cmi_sign = sg;
                  cmi_flags =
                    if !Clflags.recursive_types then [Cmi_format.Rectypes] else [];
                  cmi_crcs = imports;
                }
          in
          let _crc = output_cmi filename oc cmi in
          () (* don't need this crc ? *)
    end;
    let source_digest = match sourcefile with Some f -> Some (Digest.file f) | None -> None in
    let cmt = {
      cmt_modname = modname;
      cmt_annots = binary_annots;
      cmt_comments = Lexer.comments ();
      cmt_args = Sys.argv;
      cmt_sourcefile = (match sourcefile with Some f -> f | None -> filename);
      cmt_builddir =  Sys.getcwd ();
      cmt_loadpath = !Config.load_path;
      cmt_packed = packed_modules;
      cmt_source_digest = source_digest;
      cmt_initial_env = initial_env;
(* TODO
      cmt_crcs = crcs;
      cmt_flags = [];
*)
    } in
    output_cmt oc cmt;
    close_out oc;
    set_saved_types [];
  end;
  set_saved_types  []
