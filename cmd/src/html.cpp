#include "html.hpp"

namespace html {
    void gen_html(std::ostream& out, const std::shared_ptr<cmd::Node>& node) {
        out << "<!DOCTYPE html>\n";
        out << "<html>\n";
        out << "<head>\n";
        out << "  <meta charset='utf-8'>\n";
        out << "  <title>Shell Script AST Visualization</title>\n";
        out << "  <style>\n";
        out << "    body { font-family: 'Courier New', monospace; margin: 20px; background-color: #0a0a0a; color: #e0e0e0; }\n";
        out << "    h1, h2, h3, h4, h5 { color: #ff3333; margin: 5px 0; text-shadow: 1px 1px 2px rgba(0,0,0,0.8); }\n";
        out << "    .node { margin: 10px 0; padding: 10px; border-radius: 5px; background-color: #1a1a1a; border: 1px solid #333; }\n";
        out << "    table { border-collapse: collapse; width: auto; margin: 10px 0; border: 0px solid #444; background-color: #151515; }\n";
        out << "    th { background-color: #2a0000; color: #ff6666; text-align: left; padding: 8px; }\n";
        out << "    td { padding: 8px; border: 1px solid #333; }\n";
        out << "    .command { background-color: #1a0000; }\n";
        out << "    .pipeline { background-color: #001a1a; }\n";
        out << "    .redirection { background-color: #1a1a00; }\n";
        out << "    .sequence { background-color: #001a1a; }\n";
        out << "    .logical-and { background-color: #1a001a; }\n";
        out << "    .if-statement { background-color: #0a1a0a; }\n";
        out << "    .while-statement { background-color: #1a0a0a; }\n";
        out << "    .for-statement { background-color: #0a0a1a; }\n";
        out << "    .variable-assignment { background-color: #1a1a0a; }\n";
        out << "    .string-literal { background-color: #1a0a00; }\n";
        out << "    .number-literal { background-color: #001a0a; }\n";
        out << "    .variable-reference { background-color: #0a001a; }\n";
        out << "    .binary-expression { background-color: #1a000a; }\n";
        out << "    .unary-expression { background-color: #00001a; }\n";
        out << "    .command-substitution { background-color: #0a1a00; }\n";
        out << "    .filename { color: #ffaa33; font-style: italic; }\n";
        out << "    .operator { color: #ff9999; font-weight: bold; }\n";
        out << "    .literal { color: #66ff66; }\n";
        out << "    .variable { color: #66ffff; }\n";
        out << "    .symbol { color: #ff6666; font-weight: bold; text-align: center; }\n";
        out << "  </style>\n";
        out << "</head>\n";
        out << "<body>\n";
        out << "  <h1>Shell Script AST Visualization</h1>\n";   
        node->print(out, 0);
        out << "</body>\n";
        out << "</html>\n";
    }
}