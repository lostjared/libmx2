# list all files in directory
# extract filename and extension

files = $(ls)
for i in files; do
  e = $(strfind 0 i ".")
  if test e --ne 0; then
    length = $(strlen i)
    pos = length - e - 1
    spot  = e + 1
    ext = $(index i spot pos)
    filename = $(index i 0 e)
    printf "found filename %s of type: %s\n" filename ext
  fi
done
