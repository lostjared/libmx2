x = 0
while test x --lt 10; do
printf "%d\n" x
x=x+1
if test x --gt 5; then
break
fi
done

x = 0
while test x --lt 10; do
x = x + 1
if test x --lt 5; then
continue
fi
printf "hey: %d\n" x
 
done

files = $(ls)
x = 0
for i in files; do
x = x + 1
if test x --gt 5; then
break
fi
printf "%s\n" i
done

x = 0
for i in files; do
x = x + 1
if test x --lt 5; then
continue
fi
printf "%s\n" i
done
