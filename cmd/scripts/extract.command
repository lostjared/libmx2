
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

printf "Printing direcotries: "
list_dir "."
printf "Printing files: "
list_files "."
