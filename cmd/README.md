# CMD - Shell Interpreter / Console for MX2

![image](https://github.com/user-attachments/assets/b7650af0-770b-406f-a9a0-9e17305b95d4)

## Features

- **File Operations**: Manage files/directories (cat, cp, mv, mkdir, etc.)
- **Text Processing**: Filter, search, and manipulate text (grep, sed, sort, wc)
- **System Interaction**: Navigate and inspect the environment (cd, pwd, ls)
- **Debugging Utilities**: Set/get variables, formatted printing, state management

## Commands Reference

| Command       | Arguments                          | Description                                                                 |
|---------------|------------------------------------|-----------------------------------------------------------------------------|
| `exit`        | *None*                             | Terminates the program immediately.                                        |
| `echo`        | `<text...>`                        | Outputs all arguments separated by spaces.                                 |
| `cat`         | `[file...]`                        | Concatenates files or stdin to stdout. Missing files show errors.          |
| `grep`        | `[-r\|-e] <pattern> [file...]`     | Searches for patterns (literal or regex). Supports file/list input.        |
| `print`       | `[-i\|-f] <variable...>`           | Prints variables (-i for integer, -f for string). Handles type conversion. |
| `cd`          | `<path>`                           | Changes current directory. Shows errors for invalid paths.                 |
| `ls`          | `[path]`                           | Lists directory contents (default: current dir).                           |
| `sort`        | `[file...]`                        | Sorts lines from files/stdin lexicographically.                            |
| `find`        | `<path> <pattern>`                 | Recursively searches for filenames containing pattern.                     |
| `pwd`         | *None*                             | Prints current working directory path.                                     |
| `mkdir`       | `[-p] <dir...>`                    | Creates directories (-p creates parent dirs if needed).                    |
| `cp`          | `[-r] <source...> <dest>`          | Copies files (-r for recursive directory copy).                            |
| `mv`          | `<source...> <dest>`               | Moves/renames files. Multiple sources require dest to be directory.        |
| `touch`       | `<file...>`                        | Creates empty files or updates timestamps.                                 |
| `wc`          | `[-lwc] [file...]`                 | Counts lines/words/chars. Shows totals for multiple files.                 |
| `chmod`       | `<mode> <file...>`                 | Changes file permissions using octal mode (e.g., 755).                     |
| `sed`         | `[-n] [-i] 's/pat/repl/[g]' [file]`| Stream editor with regex substitution. Supports in-place editing (-i).     |
| `printf`      | `<format> [args...]`               | Formatted printing with escape sequences and variable expansion.           |
| `debug_set`   | `<var> <value>`                    | Stores a variable in the GameState.                                        |
| `debug_get`   | `<var>`                            | Retrieves a stored variable value.                                         |
| `debug_list`  | *None*                             | Lists all stored debug variables.                                          |
| `debug_clear` | `[var]`                            | Clears specific variable or all variables if no arg provided.              |

## Key Implementation Details

- **Regex Support**: `grep` and `sed` use C++ `<regex>` with ECMAScript syntax
- **Filesystem**: Utilizes C++17 `<filesystem>` for cross-platform path handling
- **State Management**: `debug` commands interact with a singleton `GameState`
- **Error Handling**: Commands return 0/1 exit codes and show descriptive errors
- **Streams**: Supports both file I/O and standard stream redirection
