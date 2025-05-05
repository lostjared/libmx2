
define list_dir(args)
begin
   files = $(ls args)
   for i in files
   do
      full_path = $(printf "%s/%s" args i)
      if test --d full_path; then
         printf "%s is a directory.\n" i
      else
         printf "%s is a file.\n" i
      fi
   done
   return 0
end

list_dir "."