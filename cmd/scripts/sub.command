define echo_all(ext)
begin
for i in $(ls | grep --r $(printf "%s$" ext))
do
printf "file: /***%s***/\n" i
exec $(printf "cat %s" i)
done
end

echo_all ".c"
