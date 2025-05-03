let hello = "Hello, World!"
let loc = 0
let val = 1
let hello_len = $(strlen hello)
while test loc -lt hello_len; do
let letter = $(index hello loc val)
printf "Letter is: %c\n" letter
let loc = loc + 1 
done
