let ()  =
  Dynlink.init ();
  for i = 1 to Array.length Sys.argv - 1 do
    let name = Sys.argv.(i) in
    Printf.printf "Loading %s\n" name; flush stdout;
    try 
      if name.[0] = '-'
      then Dynlink.loadfile_private 
	(String.sub name 1 (String.length name - 1))
      else Dynlink.loadfile name
    with
      | Dynlink.Error err ->
	  Printf.eprintf "Dynlink error: %s\n" 
	    (Dynlink.error_message err)
      | exn ->
	  Printf.eprintf "Error: %s\n" (Printexc.to_string exn)
  done


