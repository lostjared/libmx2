all:
	em++ -std=c++20 -O2 -I./include src/page.cpp src/scanner.cpp src/ast.cpp src/command_reg.cpp src/command.cpp src/html.cpp src/string_buffer.cpp src/types.cpp -lembind -sMODULARIZE -sEXPORT_ES6=1 -o export_html.js
