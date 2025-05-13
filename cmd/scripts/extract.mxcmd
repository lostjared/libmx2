
define list_dir(args)
begin
   files = $(ls args)
   for i in files
   do
      full_path = $(printf "%s/%s" args i)
      if test --d full_path
      then
         printf "%s\n" i
      fi
   done
   return 0
end

define list_files(args)
begin
   files = $(ls args)
   for i in files
   do
      full_path = $(printf "%s/%s" args i)
      if test --f full_path
      then
        printf "%s\n" i
      fi
   done
   return 0
end


define list_files_ext(args)
begin
   files = $(ls args)
   for i in files
   do
      full_path = $(printf "%s/%s" args i)
      if test --f full_path
      then
        pos = $(strfind 0 i ".")
        if test pos --ne  0
        then
            pos_len = $(strlen i)
            pos_end = pos_len - pos - 1
            pos++
            ext = $(index i pos pos_len)
            printf "extension: %s\n" ext      
        fi
      fi
   done
   return 0
end

define main()
begin
   printf "Printing direcotries: "
   list_dir "."
   printf "Printing files: "
   list_files "."
   printf "Printing extensions: "
   list_files_ext "."
   return 0
end

main 
