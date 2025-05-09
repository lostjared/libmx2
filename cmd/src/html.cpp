#include "html.hpp"

namespace html {
    // thanks claude for this interface. 
    void gen_html(std::ostream& out, const std::shared_ptr<cmd::Node>& node) {
        out << "<!DOCTYPE html>\n";
        out << "<html>\n";
        out << "<head>\n";
        out << "  <meta charset='utf-8'>\n";
        out << "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
        out << "  <title>Shell AST Visualization</title>\n";
        out << "  <link rel='preconnect' href='https://fonts.googleapis.com'>\n";
        out << "  <link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>\n";
        out << "  <link href='https://fonts.googleapis.com/css2?family=Fira+Code:wght@400;500;600&family=Inter:wght@400;500;600;700&display=swap' rel='stylesheet'>\n";
        out << "  <style>\n";
        out << "    :root {\n";
        out << "      --bg-primary: #0f172a;\n";
        out << "      --bg-secondary: #1e293b;\n";
        out << "      --bg-tertiary: #334155;\n";
        out << "      --text-primary: #f8fafc;\n";
        out << "      --text-secondary: #cbd5e1;\n";
        out << "      --accent-primary: #38bdf8;\n";
        out << "      --accent-secondary: #0ea5e9;\n";
        out << "      --accent-tertiary: #0284c7;\n";
        out << "      --highlight: #2563eb;\n";
        out << "      --success: #10b981;\n";
        out << "      --warning: #f59e0b;\n";
        out << "      --error: #ef4444;\n";
        out << "      --border-radius: 8px;\n";
        out << "      --shadow-sm: 0 1px 2px 0 rgba(0, 0, 0, 0.05);\n";
        out << "      --shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);\n";
        out << "      --shadow-lg: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05);\n";
        out << "    }\n";
        out << "    * {\n";
        out << "      margin: 0;\n";
        out << "      padding: 0;\n";
        out << "      box-sizing: border-box;\n";
        out << "    }\n";
        out << "    body {\n";
        out << "      font-family: 'Inter', system-ui, -apple-system, sans-serif;\n";
        out << "      background-color: var(--bg-primary);\n";
        out << "      color: var(--text-primary);\n";
        out << "      line-height: 1.6;\n";
        out << "      padding: 2rem;\n";
        out << "      transition: all 0.3s ease;\n";
        out << "    }\n";
        out << "    header {\n";
        out << "      margin-bottom: 2rem;\n";
        out << "      border-bottom: 1px solid var(--bg-tertiary);\n";
        out << "      padding-bottom: 1rem;\n";
        out << "    }\n";
        out << "    h1 {\n";
        out << "      font-size: 2.25rem;\n";
        out << "      font-weight: 700;\n";
        out << "      color: var(--accent-primary);\n";
        out << "      margin-bottom: 0.5rem;\n";
        out << "      letter-spacing: -0.025em;\n";
        out << "    }\n";
        out << "    h2, h3, h4, h5 {\n";
        out << "      color: var(--accent-primary);\n";
        out << "      font-weight: 600;\n";
        out << "      margin: 1rem 0 0.5rem 0;\n";
        out << "      letter-spacing: -0.01em;\n";
        out << "    }\n";
        out << "    h3 {\n";
        out << "      font-size: 1.25rem;\n";
        out << "      font-weight: 500;\n";
        out << "    }\n";
        out << "    .node {\n";
        out << "      margin: 1rem 0;\n";
        out << "      padding: 1.25rem;\n";
        out << "      border-radius: var(--border-radius);\n";
        out << "      background-color: var(--bg-secondary);\n";
        out << "      border: 1px solid var(--bg-tertiary);\n";
        out << "      box-shadow: var(--shadow);\n";
        out << "      transition: all 0.2s ease-in-out;\n";
        out << "    }\n";
        out << "    .node:hover {\n";
        out << "      transform: translateY(-2px);\n";
        out << "      box-shadow: var(--shadow-lg);\n";
        out << "    }\n";
        out << "    table {\n";
        out << "      border-collapse: separate;\n";
        out << "      border-spacing: 0;\n";
        out << "      width: auto; /* Allow table to expand based on content */\n";
        out << "      min-width: 100%; /* Ensure it takes at least full width */\n";
        out << "      margin: 1rem 0;\n";
        out << "      border-radius: var(--border-radius);\n";
        out << "      overflow: visible; /* Allow content to overflow visually */\n";
        out << "      box-shadow: var(--shadow-sm);\n";
        out << "      table-layout: auto; /* Let browser determine column widths */\n";
        out << "    }\n";
        out << "    th, td {\n";
        out << "      white-space: nowrap; /* Prevent text wrapping */\n";
        out << "      vertical-align: top;\n";
        out << "      overflow: visible; /* Allow content to overflow cell bounds */\n";
        out << "    }\n";
        out << "    th {\n";
        out << "      background-color: var(--bg-tertiary);\n";
        out << "      color: var(--text-primary);\n";
        out << "      text-align: left;\n";
        out << "      padding: 0.75rem 1rem;\n";
        out << "      font-weight: 600;\n";
        out << "      text-transform: uppercase;\n";
        out << "      font-size: 0.75rem;\n";
        out << "      letter-spacing: 0.05em;\n";
        out << "    }\n";
        out << "    td {\n";
        out << "      padding: 0.75rem 1rem;\n";
        out << "      border-top: 1px solid var(--bg-tertiary);\n";
        out << "      font-family: 'Fira Code', monospace;\n";
        out << "    }\n";
        out << "    /* Add a container for horizontal scrolling */\n";
        out << "    .table-container {\n";
        out << "      width: 100%;\n";
        out << "      overflow-x: auto; /* Enable horizontal scrollbar if needed */\n";
        out << "      margin: 1rem 0;\n";
        out << "    }\n";
        out << "    /* Node type styles */\n";
        out << "    .command { background-color: rgba(56, 189, 248, 0.1); border-left: 3px solid var(--accent-primary); }\n";
        out << "    .pipeline { background-color: rgba(16, 185, 129, 0.1); border-left: 3px solid var(--success); }\n";
        out << "    .redirection { background-color: rgba(245, 158, 11, 0.1); border-left: 3px solid var(--warning); }\n";
        out << "    .sequence { background-color: rgba(99, 102, 241, 0.1); border-left: 3px solid #6366f1; }\n";
        out << "    .logical-and { background-color: rgba(139, 92, 246, 0.1); border-left: 3px solid #8b5cf6; }\n";
        out << "    .if-statement { background-color: rgba(236, 72, 153, 0.1); border-left: 3px solid #ec4899; }\n";
        out << "    .while-statement { background-color: rgba(248, 113, 113, 0.1); border-left: 3px solid #f87171; }\n";
        out << "    .for-statement { background-color: rgba(251, 146, 60, 0.1); border-left: 3px solid #fb923c; }\n";
        out << "    .variable-assignment { background-color: rgba(20, 184, 166, 0.1); border-left: 3px solid #14b8a6; }\n";
        out << "    .string-literal { background-color: rgba(16, 185, 129, 0.1); border-left: 3px solid #10b981; }\n";
        out << "    .number-literal { background-color: rgba(56, 189, 248, 0.1); border-left: 3px solid #38bdf8; }\n";
        out << "    .variable-reference { background-color: rgba(139, 92, 246, 0.1); border-left: 3px solid #8b5cf6; }\n";
        out << "    .binary-expression { background-color: rgba(236, 72, 153, 0.1); border-left: 3px solid #ec4899; }\n";
        out << "    .unary-expression { background-color: rgba(248, 113, 113, 0.1); border-left: 3px solid #f87171; }\n";
        out << "    .command-substitution { background-color: rgba(251, 146, 60, 0.1); border-left: 3px solid #fb923c; }\n";
        out << "    .return { background-color: rgba(234, 88, 12, 0.1); border-left: 3px solid #ea580c; }\n";
        out << "    /* Text styles */\n";
        out << "    .filename { color: #fbbf24; font-style: italic; }\n";
        out << "    .operator { color: #e11d48; font-weight: bold; }\n";
        out << "    .literal { color: #10b981; }\n";
        out << "    .variable { color: #38bdf8; }\n";  
        out << "    .symbol { color: #ec4899; font-weight: 500; }\n";
        out << "    /* Responsive design */\n";
        out << "    @media (max-width: 768px) {\n";
        out << "      body { padding: 1rem; }\n";
        out << "      h1 { font-size: 1.75rem; }\n";
        out << "      .node { padding: 1rem; }\n";
        out << "    }\n";
        out << "    /* Collapsible sections */\n";
        out << "    .node .value, .node .body, .node .right, .node .left {\n";
        out << "      position: relative;\n";
        out << "      padding-left: 1.25rem;\n";
        out << "      display: block;\n";
        out << "    }\n";
        out << "    .node h3 {\n";
        out << "      cursor: pointer;\n";
        out << "      user-select: none;\n";
        out << "      display: flex;\n";
        out << "      align-items: center;\n";
        out << "    }\n";
        out << "    .node h3::before {\n";
        out << "      content: '▼';\n";
        out << "      display: inline-block;\n";
        out << "      margin-right: 0.5rem;\n";
        out << "      font-size: 0.75rem;\n";
        out << "      transition: transform 0.2s ease;\n";
        out << "      transform: rotate(var(--rotate, 0deg));\n";
        out << "    }\n";
        out << "    .node h3.collapsed::before {\n";
        out << "      transform: rotate(-90deg);\n";
        out << "    }\n";
        out << "  </style>\n";
        out << "  <script>\n";
        out << "    document.addEventListener('DOMContentLoaded', function() {\n";
        out << "      // Wrap tables in scrollable containers\n";
        out << "      document.querySelectorAll('main > table, .node > table').forEach(table => {\n";
        out << "        // Check if it's already wrapped\n";
        out << "        if (!table.parentElement || !table.parentElement.classList.contains('table-container')) {\n";
        out << "          const container = document.createElement('div');\n";
        out << "          container.className = 'table-container';\n";
        out << "          // Insert the container before the table\n";
        out << "          table.parentNode.insertBefore(container, table);\n";
        out << "          // Move the table inside the container\n";
        out << "          container.appendChild(table);\n";
        out << "        }\n";
        out << "        // Force reflow might still be useful\n";
        out << "        void table.offsetHeight;\n";
        out << "      });\n\n";
        out << "      // Existing collapsible logic\n";
        out << "      document.querySelectorAll('.node h3').forEach(header => {\n";
        out << "        header.addEventListener('click', function() {\n";
        out << "          const content = this.nextElementSibling;\n";
        out << "          if (content) {\n";
        out << "            const isVisible = getComputedStyle(content).display !== 'none';\n";
        out << "            content.style.display = isVisible ? 'none' : 'block';\n";
        out << "            this.classList.toggle('collapsed', isVisible);\n";
        out << "          }\n";
        out << "        });\n";
        out << "      });\n";
        out << "    });\n";
        out << "  </script>\n";
        out << "</head>\n";
        out << "<body>\n";
        out << "  <header>\n";
        out << "    <h1>Shell AST Visualization</h1>\n";
        out << "    <p>Interactive abstract syntax tree representation of your shell script</p>\n";
        out << "  </header>\n";
        out << "  <main>\n";
        node->print(out, 4);
        out << "  </main>\n";
        out << "</body>\n";
        out << "</html>\n";
    }

    void gen_html_color(std::ostream &out, std::shared_ptr<cmd::Node> &node) {
        out << "<!DOCTYPE html>\n";
        out << "<html>\n";
        out << "<head>\n";
        out << "  <meta charset='utf-8'>\n";
        out << "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
        out << "  <title>MXCMD Code Syntax Highlighting</title>\n";
        out << "  <link rel='preconnect' href='https://fonts.googleapis.com'>\n";
        out << "  <link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>\n";
        out << "  <link href='https://fonts.googleapis.com/css2?family=Fira+Code:wght@400;500;600&family=Inter:wght@400;500;600;700&display=swap' rel='stylesheet'>\n";
        out << "  <style>\n";
        out << "    :root {\n";
        out << "      --bg-primary: #0f172a;\n";
        out << "      --bg-secondary: #1e293b;\n";
        out << "      --text-primary: #f8fafc;\n";
        out << "      --border-color: #334155;\n";
        out << "      --keyword: #ec4899;\n";       // Pink for keywords
        out << "      --function: #38bdf8;\n";      // Blue for functions
        out << "      --variable: #67e8f9;\n";      // Cyan for variables
        out << "      --number: #10b981;\n";        // Green for numbers
        out << "      --string: #fbbf24;\n";        // Yellow for strings
        out << "      --comment: #64748b;\n";       // Slate for comments
        out << "      --operator: #e11d48;\n";      // Red for operators
        out << "      --punctuation: #cbd5e1;\n";   // Light gray for punctuation
        out << "      --identifier: #f8fafc;\n";    // White for identifiers
        out << "      --bracket: #94a3b8;\n";       // Gray for brackets
        out << "      --shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);\n";
        out << "    }\n";
        out << "    * {\n";
        out << "      margin: 0;\n";
        out << "      padding: 0;\n";
        out << "      box-sizing: border-box;\n";
        out << "    }\n";
        out << "    body {\n";
        out << "      font-family: 'Inter', system-ui, -apple-system, sans-serif;\n";
        out << "      background-color: var(--bg-primary);\n";
        out << "      color: var(--text-primary);\n";
        out << "      line-height: 1.6;\n";
        out << "      padding: 2rem;\n";
        out << "      transition: all 0.3s ease;\n";
        out << "    }\n";
        out << "    header {\n";
        out << "      margin-bottom: 2rem;\n";
        out << "      border-bottom: 1px solid var(--border-color);\n";
        out << "      padding-bottom: 1rem;\n";
        out << "    }\n";
        out << "    h1 {\n";
        out << "      font-size: 2.25rem;\n";
        out << "      font-weight: 700;\n";
        out << "      color: var(--function);\n";
        out << "      margin-bottom: 0.5rem;\n";
        out << "    }\n";
        out << "    .code-container {\n";
        out << "      background-color: var(--bg-secondary);\n";
        out << "      border-radius: 8px;\n";
        out << "      padding: 1.5rem;\n";
        out << "      box-shadow: var(--shadow);\n";
        out << "      margin-bottom: 2rem;\n";
        out << "      overflow-x: auto;\n";
        out << "    }\n";
        out << "    pre.code {\n";
        out << "      font-family: 'Fira Code', monospace;\n";
        out << "      font-size: 14px;\n";
        out << "      line-height: 1.2;\n";  
        out << "      white-space: pre;\n";
        out << "    }\n";
        out << "    /* Syntax highlighting classes */\n";
        out << "    .keyword { color: var(--keyword); }\n";
        out << "    .function { color: var(--function); }\n";
        out << "    .variable { color: var(--variable); }\n";
        out << "    .string { color: var(--string); }\n";
        out << "    .number { color: var(--number); }\n";
        out << "    .comment { color: var(--comment); font-style: italic; }\n";
        out << "    .operator { color: var(--operator); }\n";
        out << "    .punctuation { color: var(--punctuation); }\n";
        out << "    .identifier { color: var(--identifier); }\n";
        out << "    .bracket { color: var(--bracket); }\n";
        out << "    .line-number {\n";
        out << "      display: inline-block;\n";
        out << "      width: 2rem;\n";
        out << "      padding-right: 1rem;\n";
        out << "      text-align: right;\n";
        out << "      color: var(--comment);\n";
        out << "      user-select: none;\n";
        out << "    }\n";
        out << "    .code-line {\n";
        out << "      display: inline-block;\n";  // Change from block to inline-block
        out << "    }\n";
        out << "  </style>\n";
        out << "</head>\n";
        out << "<body>\n";
        out << "  <header>\n";
        out << "    <h1>MXCMD Syntax Highlighting</h1>\n";
        out << "    <p>Colorized code representation</p>\n";
        out << "  </header>\n";
        out << "  <div class=\"code-container\">\n";
        out << "    <pre class=\"code\">\n";
        
        class CodeFormatter {
        public:
            static std::string escapeHtml(const std::string& input) {
                std::string escaped;
                escaped.reserve(input.size());
                for (char c : input) {
                    switch (c) {
                        case '<': escaped += "&lt;"; break;
                        case '>': escaped += "&gt;"; break;
                        case '&': escaped += "&amp;"; break;
                        case '"': escaped += "&quot;"; break;
                        case '\'': escaped += "&#39;"; break;
                        default: escaped += c;
                    }
                }
                return escaped;
            }
            
            static std::string formatEscapeSequences(const std::string& input) {
                std::string result;
                result.reserve(input.size() * 2);
                
                for (size_t i = 0; i < input.size(); ++i) {
                    switch (input[i]) {
                        case '\n': result += "\\n"; break;
                        case '\t': result += "\\t"; break;
                        case '\r': result += "\\r"; break;
                        case '\f': result += "\\f"; break;
                        case '\v': result += "\\v"; break;
                        case '\b': result += "\\b"; break;
                        case '\\': result += "\\\\"; break;
                        default: result += input[i]; break;
                    }
                }
                
                return result;
            }
            
            static void format(std::ostream& out, const std::shared_ptr<cmd::Node>& node) {
                std::vector<std::string> formattedLines;
                formatBlock(formattedLines, node, 0);
                for (size_t i = 0; i < formattedLines.size(); ++i) {
                    out << "<span class=\"line-number\">" << (i + 1) << "</span>";
                    out << formattedLines[i] << "\n";
                }
            }
            
            static std::string formatNodeInline(const std::shared_ptr<cmd::Node>& node) {
                if (!node) {
                    return "<span class=\"comment\">/* null */</span>";
                }
                
                if (auto expr = std::dynamic_pointer_cast<cmd::Expression>(node)) {
                    return formatExpressionInline(expr);
                }
                
                if (auto cmd = std::dynamic_pointer_cast<cmd::Command>(node)) {
                    std::string result = "<span class=\"identifier\">" + escapeHtml(cmd->name) + "</span>";
                    
                    for (size_t arg_index = 0; arg_index < cmd->args.size(); arg_index++) {
                        const auto& arg = cmd->args[arg_index];
                        result += " ";
                        
                        if (arg.type == cmd::ARG_VARIABLE) {
                            result += "<span class=\"variable\">" + escapeHtml(arg.value) + "</span>";
                        } else if (arg.type == cmd::ARG_STRING_LITERAL) {
                            result += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                        } else if (cmd->name == "printf" || cmd->name == "echo" || cmd->name == "puts" || 
                                   cmd->name == "print" || cmd->name == "say") {
                            if (arg_index == 0) {
                                result += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                            } else {
                                if (arg.type == cmd::ARG_VARIABLE) {
                                    result += "<span class=\"variable\">" + escapeHtml(arg.value) + "</span>";
                                } else if (arg.value.find_first_of("0123456789") == 0 && 
                                          arg.value.find_first_not_of("0123456789.") == std::string::npos) {
                                    result += "<span class=\"number\">" + escapeHtml(std::to_string(std::stod(arg.value))) + "</span>";
                                } else {
                                    result += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                                }
                            }
                        } else {
                            if (arg.type == cmd::ARG_VARIABLE) {
                                result += "<span class=\"variable\">" + escapeHtml(arg.value) + "</span>";
                            } else if (arg.value.find(' ') != std::string::npos ||
                                      arg.value.find('\t') != std::string::npos ||
                                      arg.value.find('\n') != std::string::npos ||
                                      arg.value.find('\\') != std::string::npos ||
                                      arg.value.find('%') != std::string::npos ||
                                      arg.value.find('"') != std::string::npos ||
                                      arg.value.find('\'') != std::string::npos) {
                                result += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                            } else if (arg.value.find_first_of("0123456789") == 0 && 
                                      arg.value.find_first_not_of("0123456789.") == std::string::npos) {
                                result += "<span class=\"number\">" + escapeHtml(std::to_string(std::stod(arg.value.c_str()))) + "</span>";
                            } else {
                                result += "<span class=\"identifier\">" + escapeHtml(arg.value) + "</span>";
                            }
                        }
                    }
                    
                    return result;
                }
                
                if (auto seq = std::dynamic_pointer_cast<cmd::Sequence>(node)) {
                    std::string result;
                    for (size_t i = 0; i < seq->commands.size(); i++) {
                        if (i > 0) result += "; ";
                        result += formatNodeInline(seq->commands[i]);
                    }
                    return result;
                }
                
                return "<span class=\"comment\">/* complex condition */</span>";
            }
            
        private:
            static void formatBlock(std::vector<std::string>& lines, const std::shared_ptr<cmd::Node>& node, int indentLevel) {
                std::string indent(indentLevel * 2, ' ');
                
                if (!node) {
                    lines.push_back("<span class=\"code-line\">" + indent + 
                                   "<span class=\"comment\">// null node</span></span>");
                    return;
                }
                
                if (auto seq = std::dynamic_pointer_cast<cmd::Sequence>(node)) {
                    for (const auto& cmd : seq->commands) {
                        formatBlock(lines, cmd, indentLevel);
                    }
                }
                else if (auto cmd = std::dynamic_pointer_cast<cmd::Command>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    line += "<span class=\"identifier\">" + escapeHtml(cmd->name) + "</span>";
                    
                    for (size_t arg_index = 0; arg_index < cmd->args.size(); arg_index++) {
                        const auto& arg = cmd->args[arg_index];
                        line += " ";
                        
                        if (arg.type == cmd::ARG_VARIABLE) {
                            line += "<span class=\"variable\">" + escapeHtml(arg.value) + "</span>";
                        } else if (arg.type == cmd::ARG_STRING_LITERAL) {
                            line += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                        } else if (arg.value.find_first_of("0123456789") == 0 && 
                                  arg.value.find_first_not_of("0123456789.") == std::string::npos) {
                            line += "<span class=\"number\">" + escapeHtml(std::to_string(std::stod(arg.value))) + "</span>";
                        } else if (arg.value.find(' ') != std::string::npos ||
                                  arg.value.find('\t') != std::string::npos ||
                                  arg.value.find('\n') != std::string::npos ||
                                  arg.value.find('\\') != std::string::npos ||
                                  arg.value.find('%') != std::string::npos ||
                                  arg.value.find('"') != std::string::npos ||
                                  arg.value.find('\'') != std::string::npos) {
                            line += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                        } else {
                            line += "<span class=\"identifier\">" + escapeHtml(arg.value) + "</span>";
                        }
                    }
                    line += "</span>";
                    lines.push_back(line);
                }
                else if (auto varAssign = std::dynamic_pointer_cast<cmd::VariableAssignment>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    line += "<span class=\"variable\">" + escapeHtml(varAssign->name) + "</span> ";
                    line += "<span class=\"operator\">=</span> ";
                    
                    if (auto exprNode = std::dynamic_pointer_cast<cmd::Expression>(varAssign->value)) {
                        line += formatExpressionInline(exprNode);
                    } else {
                        line += "<span class=\"comment\">/* non-expression value */</span>";
                    }
                    line += "</span>";
                    lines.push_back(line);
                }
                else if (auto ifStmt = std::dynamic_pointer_cast<cmd::IfStatement>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    line += "<span class=\"keyword\">if</span> ";
                    if (!ifStmt->branches.empty()) {
                        line += formatNodeInline(ifStmt->branches[0].condition);
                    }
                    line += "</span>";
                    lines.push_back(line);
                    
                    lines.push_back("<span class=\"code-line\">" + indent + 
                                   "<span class=\"keyword\">then</span></span>");
                    
                    if (!ifStmt->branches.empty()) {
                        formatBlock(lines, ifStmt->branches[0].action, indentLevel + 2);
                    }
                    
                    for (size_t i = 1; i < ifStmt->branches.size(); i++) {
                        std::string elifLine = "<span class=\"code-line\">" + indent;
                        elifLine += "<span class=\"keyword\">elif</span> ";
                        
                        elifLine += formatNodeInline(ifStmt->branches[i].condition);
                        elifLine += "</span>";
                        lines.push_back(elifLine);
                        
                        lines.push_back("<span class=\"code-line\">" + indent + 
                                       "<span class=\"keyword\">then</span></span>");
                        
                        formatBlock(lines, ifStmt->branches[i].action, indentLevel + 2);
                    }
                    
                    
                    if (ifStmt->elseAction) { 
                        lines.push_back("<span class=\"code-line\">" + indent + 
                                       "<span class=\"keyword\">else</span></span>");
                        formatBlock(lines, ifStmt->elseAction, indentLevel + 2);
                    }
                    
                    
                    lines.push_back("<span class=\"code-line\">" + indent + 
                                   "<span class=\"keyword\">fi</span></span>");
                }
                else if (auto forStmt = std::dynamic_pointer_cast<cmd::ForStatement>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    line += "<span class=\"keyword\">for</span> ";
                    line += "<span class=\"variable\">" + escapeHtml(forStmt->variable) + "</span> ";
                    line += "<span class=\"keyword\">in</span> ";
                    
                    for (const auto& val : forStmt->values) {
                        if (val.type == cmd::ARG_VARIABLE) {
                            line += "<span class=\"variable\">" + escapeHtml(val.value) + "</span> ";
                        } else {
                            line += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(val.value)) + "&quot;</span> ";
                        }
                    }
                    line += "</span>";
                    lines.push_back(line);
                    
                    
                    lines.push_back("<span class=\"code-line\">" + indent + 
                                   "<span class=\"keyword\">do</span></span>");
                    
                    
                    formatBlock(lines, forStmt->body, indentLevel + 2);
                    
                    
                    lines.push_back("<span class=\"code-line\">" + indent + 
                                   "<span class=\"keyword\">done</span></span>");
                }
                else if (auto redirNode = std::dynamic_pointer_cast<cmd::Redirection>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    
                    
                    if (auto cmd = std::dynamic_pointer_cast<cmd::Command>(redirNode->command)) {
                        line += "<span class=\"identifier\">" + escapeHtml(cmd->name) + "</span>";
                        
                        for (size_t arg_index = 0; arg_index < cmd->args.size(); arg_index++) {
                            const auto& arg = cmd->args[arg_index];
                            line += " ";
                            
                            if (arg.type == cmd::ARG_VARIABLE) {
                                line += "<span class=\"variable\">" + escapeHtml(arg.value) + "</span>";
                            } else if (arg.type == cmd::ARG_STRING_LITERAL) {
                                line += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                            } else if (arg.value.find_first_of("0123456789") == 0 && 
                                      arg.value.find_first_not_of("0123456789.") == std::string::npos) {
                                line += "<span class=\"number\">" + escapeHtml(std::to_string(std::stod(arg.value))) + "</span>";
                            } else if (arg.value.find(' ') != std::string::npos ||
                                      arg.value.find('\t') != std::string::npos ||
                                      arg.value.find('\n') != std::string::npos ||
                                      arg.value.find('\\') != std::string::npos ||
                                      arg.value.find('%') != std::string::npos ||
                                      arg.value.find('"') != std::string::npos ||
                                      arg.value.find('\'') != std::string::npos) {
                                line += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                            } else {
                                line += "<span class=\"identifier\">" + escapeHtml(arg.value) + "</span>";
                            }
                        }
                    } else {
                        line += "<span class=\"comment\">/* complex command */</span>";
                    }
                    
                    
                    std::string op;
                    switch (redirNode->type) {
                        case cmd::Redirection::INPUT: op = "<"; break;
                        case cmd::Redirection::OUTPUT: op = ">"; break;
                        case cmd::Redirection::APPEND: op = ">>"; break;
                        default: op = "?"; break;
                    }
                    line += " <span class=\"operator\">" + escapeHtml(op) + "</span> ";
                    
                    
                    line += "<span class=\"string\">" + escapeHtml(formatEscapeSequences(redirNode->file)) + "</span>";
                    line += "</span>";
                    lines.push_back(line);
                }
                else if (auto pipeNode = std::dynamic_pointer_cast<cmd::Pipeline>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    
                    
                    if (!pipeNode->commands.empty() && pipeNode->commands.size() >= 2) {
                    
                        if (auto cmd = pipeNode->commands[0]) {
                            line += "<span class=\"identifier\">" + escapeHtml(cmd->name) + "</span>";
                        }
                        
                        line += " <span class=\"operator\">|</span> ";
                        
                    
                        if (auto cmd = pipeNode->commands[1]) {
                            line += "<span class=\"identifier\">" + escapeHtml(cmd->name) + "</span>";
                        }
                    }
                    
                    line += "</span>";
                    lines.push_back(line);
                }
                else if (auto funcDef = std::dynamic_pointer_cast<cmd::CommandDefinition>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    line += "<span class=\"keyword\">define</span> ";  
                    line += "<span class=\"function\">" + escapeHtml(funcDef->name) + "</span>";
                    line += "<span class=\"bracket\">(</span>";
                    for (size_t i = 0; i < funcDef->parameters.size(); ++i) {
                        if (i > 0) {
                            line += "<span class=\"punctuation\">, </span>";
                        }
                        line += "<span class=\"variable\">" + escapeHtml(funcDef->parameters[i]) + "</span>";
                    }
                    line += "<span class=\"bracket\">)</span>";
                    line += "</span>";
                    lines.push_back(line);
                    
                    lines.push_back("<span class=\"code-line\">" + indent + 
                                   "<span class=\"keyword\">begin</span></span>");  
                    
                    
                    formatBlock(lines, funcDef->body, indentLevel + 2);
                    
                    
                    lines.push_back("<span class=\"code-line\">" + indent + 
                                   "<span class=\"keyword\">end</span></span>");  
                }
                else if (auto whileStmt = std::dynamic_pointer_cast<cmd::WhileStatement>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    line += "<span class=\"keyword\">while</span> ";
                    
                    if (auto condExpr = std::dynamic_pointer_cast<cmd::Expression>(whileStmt->condition)) {
                        line += formatExpressionInline(condExpr);
                    } else {
                        line += "<span class=\"comment\">/* command condition */</span>";
                    }
                    
                    line += "</span>";
                    lines.push_back(line);
                    
                    lines.push_back("<span class=\"code-line\">" + indent + 
                                   "<span class=\"keyword\">do</span></span>");
                    
                    formatBlock(lines, whileStmt->body, indentLevel + 2);
                    
                    lines.push_back("<span class=\"code-line\">" + indent + 
                                   "<span class=\"keyword\">done</span></span>");
                }
                else if (auto logicalAnd = std::dynamic_pointer_cast<cmd::LogicalAnd>(node)) {
                    formatBlock(lines, logicalAnd->left, indentLevel);
                    if (!lines.empty()) {
                        std::string lastLine = lines.back();
                        lines.pop_back();
                        lastLine += " <span class=\"operator\">&&</span> ";
                        lines.push_back(lastLine);
                    }
                    formatBlock(lines, logicalAnd->right, 0);
                }
                else if (auto unaryExpr = std::dynamic_pointer_cast<cmd::UnaryExpression>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    
                    if (unaryExpr->op == cmd::UnaryExpression::INCREMENT || 
                        unaryExpr->op == cmd::UnaryExpression::DECREMENT) {
                        
                        std::string opStr = (unaryExpr->op == cmd::UnaryExpression::INCREMENT) ? "++" : "--";
                        
                        if (unaryExpr->position == cmd::UnaryExpression::PREFIX) {
                            line += "<span class=\"operator\">" + opStr + "</span>";
                            if (auto varRef = std::dynamic_pointer_cast<cmd::VariableReference>(unaryExpr->operand)) {
                                line += "<span class=\"variable\">" + escapeHtml(varRef->name) + "</span>";
                            }
                        } else { 
                            if (auto varRef = std::dynamic_pointer_cast<cmd::VariableReference>(unaryExpr->operand)) {
                                line += "<span class=\"variable\">" + escapeHtml(varRef->name) + "</span>";
                            }
                            line += "<span class=\"operator\">" + opStr + "</span>";
                        }
                    } 
                    else if (unaryExpr->op == cmd::UnaryExpression::NEGATE) {
                        line += "<span class=\"operator\">-</span>";
                        if (auto subExpr = std::dynamic_pointer_cast<cmd::Expression>(unaryExpr->operand)) {
                            line += formatExpressionInline(subExpr);
                        }
                    }
                    
                    line += "</span>";
                    lines.push_back(line);
                }
                else if (auto returnStmt = std::dynamic_pointer_cast<cmd::Return>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    line += "<span class=\"keyword\">return</span> ";
                    
                    if (auto expr = std::dynamic_pointer_cast<cmd::Expression>(returnStmt->value)) {
                        line += formatExpressionInline(expr);
                    }
                    
                    line += "</span>";
                    lines.push_back(line);
                }
                else if (auto breakStmt = std::dynamic_pointer_cast<cmd::Break>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    line += "<span class=\"keyword\">break</span>";
                    line += "</span>";
                    lines.push_back(line);
                }
                else if (auto continueStmt = std::dynamic_pointer_cast<cmd::Continue>(node)) {
                    std::string line = "<span class=\"code-line\">" + indent;
                    line += "<span class=\"keyword\">continue</span>";
                    line += "</span>";
                    lines.push_back(line);
                }
                else {
                    
                    lines.push_back("<span class=\"code-line\">" + indent +
                                   "<span class=\"comment\">// Unsupported node type</span></span>");
                }
            }
            
            
            static std::string formatExpressionInline(const std::shared_ptr<cmd::Expression>& expr) {
                if (!expr) {
                    return "<span class=\"comment\">/* null */</span>";
                }
                if (auto strLit = std::dynamic_pointer_cast<cmd::StringLiteral>(expr)) {
                    return "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(strLit->value)) + "&quot;</span>";
                }
                else if (auto numLit = std::dynamic_pointer_cast<cmd::NumberLiteral>(expr)) {
                    return "<span class=\"number\">" + escapeHtml(std::to_string(numLit->value)) + "</span>";
                }
                else if (auto varRef = std::dynamic_pointer_cast<cmd::VariableReference>(expr)) {
                    return "<span class=\"variable\">" + escapeHtml(varRef->name) + "</span>";
                }
                else if (auto binExpr = std::dynamic_pointer_cast<cmd::BinaryExpression>(expr)) {
                    std::string result;
                    
            
                    if (auto leftExpr = std::dynamic_pointer_cast<cmd::Expression>(binExpr->left)) {
                        result += formatExpressionInline(leftExpr);
                    } else {
                        result += "<span class=\"comment\">/* ? */</span>";
                    }
                    
            
                    std::string opStr;
                    switch (binExpr->op) {
                        case cmd::BinaryExpression::ADD: opStr = "+"; break;
                        case cmd::BinaryExpression::SUBTRACT: opStr = "-"; break;
                        case cmd::BinaryExpression::MULTIPLY: opStr = "*"; break;
                        case cmd::BinaryExpression::DIVIDE: opStr = "/"; break;
                        case cmd::BinaryExpression::MODULO: opStr = "%"; break;
                        default: opStr = "?"; break;
                    }
                    result += " <span class=\"operator\">" + escapeHtml(opStr) + "</span> ";
                    
            
                    if (auto rightExpr = std::dynamic_pointer_cast<cmd::Expression>(binExpr->right)) {
                        result += formatExpressionInline(rightExpr);
                    } else {
                        result += "<span class=\"comment\">/* ? */</span>";
                    }
                    
                    return result;
                }
                else if (auto cmdSubst = std::dynamic_pointer_cast<cmd::CommandSubstitution>(expr)) {
                    std::string result = "<span class=\"operator\">$(</span>";
                    
                    if (auto cmd = std::dynamic_pointer_cast<cmd::Command>(cmdSubst->command)) {
                        result += "<span class=\"identifier\">" + escapeHtml(cmd->name) + "</span>";
                        for (size_t arg_index = 0; arg_index < cmd->args.size(); arg_index++) {
                            const auto& arg = cmd->args[arg_index];
                            result += " ";
                            
                            if (arg.type == cmd::ARG_VARIABLE) {
                                result += "<span class=\"variable\">" + escapeHtml(arg.value) + "</span>";
                            } else if (arg.type == cmd::ARG_STRING_LITERAL) {
                                result += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                            } else if (cmd->name == "printf" || cmd->name == "echo" || cmd->name == "puts" || 
                                       cmd->name == "print" || cmd->name == "say") {
                                if (arg_index == 0) {
                                    result += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                                } else {
                                    if (arg.type == cmd::ARG_VARIABLE) {
                                        result += "<span class=\"variable\">" + escapeHtml(arg.value) + "</span>";
                                    } else if (arg.value.find_first_of("0123456789") == 0 && 
                                              arg.value.find_first_not_of("0123456789.") == std::string::npos) {
                                        result += "<span class=\"number\">" + escapeHtml(std::to_string(std::stod(arg.value))) + "</span>";
                                    } else {
                                        result += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                                    }
                                }
                            } else {
                                if (arg.type == cmd::ARG_VARIABLE) {
                                    result += "<span class=\"variable\">" + escapeHtml(arg.value) + "</span>";
                                } else if (arg.value.find(' ') != std::string::npos ||
                                          arg.value.find('\t') != std::string::npos ||
                                          arg.value.find('\n') != std::string::npos ||
                                          arg.value.find('\\') != std::string::npos ||
                                          arg.value.find('%') != std::string::npos ||
                                          arg.value.find('"') != std::string::npos ||
                                          arg.value.find('\'') != std::string::npos) {
                                    result += "<span class=\"string\">&quot;" + escapeHtml(formatEscapeSequences(arg.value)) + "&quot;</span>";
                                } else if (arg.value.find_first_of("0123456789") == 0 && 
                                          arg.value.find_first_not_of("0123456789.") == std::string::npos) {
                                    result += "<span class=\"number\">" + escapeHtml(std::to_string(std::stod(arg.value))) + "</span>";
                                } else {
                                    result += "<span class=\"identifier\">" + escapeHtml(arg.value) + "</span>";
                                }
                            }
                        }
                    } else {
                        result += "<span class=\"comment\">/* cmd */</span>";
                    }
                    
                    result += "<span class=\"operator\">)</span>";
                    return result;
                }
                else if (auto unaryExpr = std::dynamic_pointer_cast<cmd::UnaryExpression>(expr)) {
                    std::string result;
                    
                    if (unaryExpr->op == cmd::UnaryExpression::INCREMENT || 
                        unaryExpr->op == cmd::UnaryExpression::DECREMENT) {
                        
                        std::string opStr = (unaryExpr->op == cmd::UnaryExpression::INCREMENT) ? "++" : "--";
                        
                        if (unaryExpr->position == cmd::UnaryExpression::PREFIX) {
                            result += "<span class=\"operator\">" + opStr + "</span>";
                            if (auto varRef = std::dynamic_pointer_cast<cmd::VariableReference>(unaryExpr->operand)) {
                                result += "<span class=\"variable\">" + escapeHtml(varRef->name) + "</span>";
                            }
                        } else { 
                            if (auto varRef = std::dynamic_pointer_cast<cmd::VariableReference>(unaryExpr->operand)) {
                                result += "<span class=\"variable\">" + escapeHtml(varRef->name) + "</span>";
                            }
                            result += "<span class=\"operator\">" + opStr + "</span>";
                        }
                    } 
                    else if (unaryExpr->op == cmd::UnaryExpression::NEGATE) {
                        result += "<span class=\"operator\">-</span>";
                        if (auto subExpr = std::dynamic_pointer_cast<cmd::Expression>(unaryExpr->operand)) {
                            result += formatExpressionInline(subExpr);
                        }
                    }
                    
                    return result;
                }
                
                return "<span class=\"comment\">/* unknown expression */</span>";
            }
        };

        CodeFormatter::format(out, node);
        
        out << "    </pre>\n";
        out << "  </div>\n";
        out << "</body>\n";
        out << "</html>\n";
    }
}