# MXCMD - Shell Interpreter / Console for MX2

![image](https://github.com/user-attachments/assets/d6662a16-7b34-4272-a069-ace2a9eb766e)
![Screenshot 2025-05-01 202552](https://github.com/user-attachments/assets/20d28250-a13d-4de6-91a3-8bcc47d445fc)

Online guide: https://lostsidedead.biz/mxcmd

Visualizer/Code Formatter: https://lostsidedead.biz/debug

# MXCMD Shell Interpreter

MXCMD Beta: a lightweight shell scripting language and command-line interpreter for Windows x64 (MSYS2/MinGW-w64), Linux, macOS, and BSD. This guide provides an overview of supported commands, control flow constructs, and expressions & syntax.

# TODO:

Create a Shared Library / module for MX2 to run in the interpreter as standalone instead of using MXCMD in the embedded terminal

# Issues:

In the embdded termianal,  commands run single threaded in the foreground thread, I need to make them run
in a background thread so the foreground thread doesn't lock up when in a loop or processing a script.


## How to build from source:
Building from Source on Linux/Unix first install the required packages, readline, git, g++, cmake.:
```bash
git clone https://github.com/lostjared/libmx2.git --depth 1
cd libmx2/cmd
mkdir build
cd build
cmake .. -DCMD=ON -DSHARED=ON
cmake --build .
sudo cmake --install .
```

Then at your shell prompt:
```bash
$ mxcmd-cli
```

## Optional compile SDL module
```bash
cd libmx2/cmd/modules/sdl
mkdir build && cd build
cmake .. 
cmake --build .
sudo  cmake --install .
```

## Table of Contents

- [Supported Commands](#supported-commands)
- [Control Flow Constructs](#control-flow-constructs)
- [Expressions & Syntax](#expressions--syntax)
- [How to Build from Source](#how-to-build-from-source)

---

## Control Flow Constructs

### If Statements
```bash
if <condition> then
    <statements>
[elif <condition> then
    <statements>]...
[else
    <statements>]
fi
```

### While Loops
```bash
while <condition> do
    <statements>
done
```

### For Loops
```bash
for <var> in <val1> <val2> ... do
    <statements>
done
```

### Break & Continue
Use `break` to exit loops and `continue` to skip to the next iteration.

### Return
Use `return <expression>` to exit a user-defined command with a status.

## Expressions & Syntax

### Literals
- Numbers: `42`, `3.14`
- Strings: `"hello"` or `'world'` **All strings like file paths have to be surrounded in quotes.**

### Variables & Assignment
```bash
<var_name> = <expression>
--<var_name>    # decrement
```

### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Unary: prefix `-`, `++`, `--`; postfix `++`, `--`

### Pipelines & Redirection
- Pipeline: `cmd1 | cmd2 | cmd3`
- Redirect output: `cmd > file`, `cmd >> file`
- Redirect input: `cmd < file`

### Logical Operators
Use `&&` and `||` for boolean logic.
```bash
pwd && pwd
ls || pwd
```
# Supported Commands

**All file paths must be surrounded in quotes**

### Jump to:
- [exec](#exec)
- [argv](#argv)
- [exit](#exit)
- [echo](#echo)
- [cat](#cat)
- [grep](#grep)
- [print](#print)
- [cd](#cd)
- [list (ls)](#list-ls)
- [sort](#sort)
- [find](#find)
- [pwd](#pwd)
- [mkdir](#mkdir)
- [cp](#cp)
- [mv](#mv)
- [touch](#touch)
- [head](#head)
- [tail](#tail)
- [wc](#wc)
- [sed](#sed)
- [printf](#printf)
- [test](#test)
- [cmd](#cmd)
- [visual](#visual)
- [at](#at)
- [len](#len)
- [index](#index)
- [strlen](#strlen)
- [strfind](#strfind)
- [strfindr](#strfindr)
- [strtok](#strtok)
- [extern](#extern)
- [List Management Commands](#list-management-commands)
- [rand](#rand)
- [expr](#expr)
- [commands](#commands)
- [Debug Commands](#debug-commands)
- [getline](#getline)
- [regex_match](#regex_match)
- [regex_replace](#regex_replace)
- [regex_search](#regex_search)
- [regex_split](#regex_split)
- [tokenize](#tokenize)

---

### exec
Execute shell command.

#### Usage
```bash
exec "ls"
```

---

### argv
Access command line arguments.

#### Usage
```bash
argv index
argv "length"
argv "all"
```

---

### exit
Terminate the shell immediately.

#### Usage
```bash
exit
```

---

### echo
Prints its arguments to standard output, expanding any variables.

#### Usage
```bash
echo [ARG]...
```

#### Arguments
- Any number of words or variables (`VAR`).

---

### cat
Concatenates and displays file contents, or reads from standard input if no files are specified.

#### Usage
```bash
cat [FILE]...
```

---

### grep
Searches input or files for lines matching a pattern, optionally using regular expressions.

#### Usage
```bash
grep [--r|--e] PATTERN [FILE]...
```

#### Options
- `--r` or `--e`: enable extended regex matching.

---

### print
Prints each argument on its own line; can convert values to integers.

#### Usage
```bash
print [--i|--f] ARG...
```

---

### cd
Change the shell’s current working directory.

#### Usage
```bash
cd DIR
```

---

### list (ls)
List the contents of a directory.

#### Usage
```bash
ls [DIR]
```

---

### sort
Sorts lines of text from files or standard input.

#### Usage
```bash
sort [FILE]...
```

---

### find
Recursively search a directory tree for filenames containing a given pattern.

#### Usage
```bash
find PATH PATTERN
```

---

### pwd
Print the current working directory.

#### Usage
```bash
pwd
```

---

### mkdir
Create new directories.

#### Usage
```bash
mkdir [--p] DIR...
```

#### Options
- `--p`: create parent directories as needed.

---

### cp
Copy files or directories.

#### Usage
```bash
cp [--r] SOURCE... DEST
```

---

### mv
Rename or move files and directories.

#### Usage
```bash
mv SOURCE... DEST
```

---

### touch
Update file timestamps or create files if they do not exist.

#### Usage
```bash
touch FILE...
```

---

### head
Display the first part of files or input.

#### Usage
```bash
head [--n N] [FILE]...
```

---

### tail
Display the last part of files or input.

#### Usage
```bash
tail [--n N] [FILE]...
```

---

### wc
Count lines, words, and characters in files or input.

#### Usage
```bash
wc [--l] [--w] [--c] [FILE]...
```

---

### sed
Perform basic stream editing via substitution expressions.

#### Usage
```bash
sed [options] 's/pattern/replacement/[g]' [FILE]
```

#### Options
- `-n`: suppress automatic printing.
- `-i`: edit files in place.

---

### printf
Formatted printing, similar to C's `printf`.

#### Usage
```bash
printf FORMAT [ARG]...
```

---

### test
Evaluate conditional expressions, returning status 0 for true, non-zero for false.

#### Usage
```bash
test ARG...
```

---

### cmd
Execute an external script file within the shell environment.

#### Usage
```bash
cmd SCRIPT_FILE
```

---

### visual
Render a script’s AST as HTML.

#### Usage
```bash
visual SCRIPT_FILE OUTPUT_HTML
```

---

### at
Extract an element by index from a newline-separated list.

#### Usage
```bash
at LIST INDEX
```

---

### len
Count items in a list, either by lines or words.

#### Usage
```bash
len LIST
```

---

### index
Extract a substring by start position and length.

#### Usage
```bash
index STRING START LENGTH
```

---

### strlen
Get the length of a string.

#### Usage
```bash
strlen STRING
```

---

### strfind
Find the position of a substring within a string.

#### Usage
```bash
strfind START STRING SEARCH
```

---

### strfindr
Find the position of a substring within a string, searching from the right (end of string).

#### Usage
```bash
strfindr STRING SEARCH
```

---

### strtok
Split a string into tokens using a delimiter and return the result as a newline-separated list.

#### Usage
```bash
strtok STRING DELIMITER
```

---

### extern
Register an external function from a shared library as a command.

#### Usage
```bash
extern LIBRARY FUNCTION COMMAND_NAME
```

#### Example
```bash
extern "math" "add_numbers" "add"
```

---

### List Management Commands

- **list_new**: `list_new NAME` — Create a new list  
- **list_add**: `list_add NAME VALUE` — Add a value to a list  
- **list_remove**: `list_remove NAME INDEX` — INDEX — Remove an item at specified index
- **list_get**: `list_get NAME INDEX` — Get an item at specified index
- **list_set**: `list_set NAME INDEX VALUE` — Set an item at specified index
- **list_clear**: `list_clear NAME` — Clear a specific list
- **list_clearall**: `list_clearall` — Clear all lists
- **list_exists**: `list_exists NAME` — Check if a list exists
- **list_init**: `list_init NAME VALUE SIZE` — Initialize a list with a value repeated SIZE times
- **list_len**: `list_len NAME` — Get the length of a list
- **list_tokens**: `list_tokens NAME` — Display all values in a list, one per line
- **list_sort**: `list_sort NAME` — Sort a list in ascending order
- **list_reverse**: `list_reverse NAME` — Reverse the order of elements in a list
- **list_shuffle**: `list_shuffle NAME` — Randomly shuffle the elements of a list
- **list_copy**: `list_copy SOURCE DEST` — Copy a list to a new list
- **list_pop**: `list_pop NAME` — Remove and return the last element of a list
- **list_concat**: `list_concat LIST1 LIST2` — Concatenate the second list to the first

---

### rand
Generate a random number between MIN and MAX (inclusive).

#### Usage
```bash
rand MIN MAX
```

---

### expr
Evaluate an expression and return the result. Supports arithmetic, comparison, and logical operations.

#### Usage
```bash
expr EXPRESSION
```

---

### commands
List all available commands in the shell, including built-in, user-defined, and externally loaded commands.

#### Usage
```bash
commands
```

---

### Debug Commands

- **debug_set**: `debug_set <VAR> <VAL>` — Set a debug variable.
- **debug_get**: `debug_get <VAR>` — Get a debug variable.
- **debug_list**: `debug_list` — List debug variables.
- **debug_clear**: `debug_clear <VAR>` — Clear a debug variable.
- **debug_clearall**: `debug_clearall` — Clear all debug variables.
- **dump**: `dump FILENAME` — Dump variable table to a file.

---

### getline
Read a line from standard input.

#### Usage
```bash
getline variable
```

---

### regex_match
Check if a string fully matches a regular expression pattern.

#### Usage
```bash
regex_match PATTERN STRING
```

---

### regex_replace
Replace parts of a string that match a regular expression pattern.

#### Usage
```bash
regex_replace PATTERN REPLACEMENT STRING
```

---

### regex_search
Search for matches of a regular expression pattern within a string.

#### Usage
```bash
regex_search PATTERN STRING
```

---

### regex_split
Split a string using a regular expression pattern as the delimiter.

#### Usage
```bash
regex_split PATTERN "a,b,c"
```

---

### tokenize
Split a MXCMD command or program into tokens using the built in Scanner/Lexer

#### Usage
```bash
tokenize filename
```

