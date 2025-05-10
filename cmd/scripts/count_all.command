define count_text(dir)
begin
    files = $(ls | sort)
    total = 0
    for f in files
    do
        path = $(printf "%s/%s" dir f)
        pos = $(strfind 0 path ".command")
        if test pos --ne 0
        then
            count = $(wc --l path)
            total = total + count
            printf "File: %s has %d lines\n" path count
        fi
    done
    printf "total lines: %d\n" total
end

count_text "."
