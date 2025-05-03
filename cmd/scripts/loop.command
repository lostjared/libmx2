files = $(ls)
index = 0
len = $(len files)
while test index -lt len; do
f = $(at files index)
printf "file: %s\n" f
index =  index + 1
done

