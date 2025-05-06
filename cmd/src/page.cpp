#include<iostream>
#include<string>
#include<sstream>
#include"ast.hpp"
#include"html.hpp"
#include"scanner.hpp"
#include"parser.hpp"
#include<emscripten/emscripten.h>
#include<emscripten/bind.h>

class ExportHTML {
public:
    ExportHTML() = default;
    std::string outputHTML(const std::string &input) {
        try {
            scan::TString string_buffer(input);
            scan::Scanner scanner(string_buffer);
            cmd::Parser parser(scanner);
            auto ast = parser.parse();
            std::cout << "Debug Information written to: debug.html\n\n";
            std::ostringstream stream;
            html::gen_html(stream, ast);
            return stream.str();
        } catch(const scan::ScanExcept &e) {
            return "Scan Error: " + e.why() + "\n";    
        } catch(const std::exception &e) {
            return "Exception: " + std::string(e.what()) + "\n";
        } catch(...) {
            return  "Unknown Error has Occoured..\n";
        }
        return "[Unknown Error]";
    }
};

EMSCRIPTEN_BINDINGS(export_html) {
    emscripten::class_<ExportHTML>("ExportHTML")
        .constructor<>()
        .function("outputHTML", &ExportHTML::outputHTML);
}