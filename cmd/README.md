# CMD - Shell Interpreter / Console for MX2

![image](https://github.com/user-attachments/assets/7762971f-684c-4890-96c0-d2f7ee8c20b8)
![image](https://github.com/user-attachments/assets/b7650af0-770b-406f-a9a0-9e17305b95d4)


Online guide: https://lostsidedead.biz/mxcmd


## Features

- **File Operations**: Manage files/directories (cat, cp, mv, mkdir, etc.)
- **Text Processing**: Filter, search, and manipulate text (grep, sed, sort, wc)
- **System Interaction**: Navigate and inspect the environment (cd, pwd, ls)
- **Debugging Utilities**: Set/get variables, formatted printing, state management

## Key Implementation Details

- **Regex Support**: `grep` and `sed` use C++ `<regex>` with ECMAScript syntax
- **Filesystem**: Utilizes C++17 `<filesystem>` for cross-platform path handling
- **State Management**: `debug` commands interact with a singleton `GameState`
- **Error Handling**: Commands return 0/1 exit codes and show descriptive errors
- **Streams**: Supports both file I/O and standard stream redirection
