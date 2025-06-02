#include"mx2-ollama.hpp"

int main(int argc, char **argv) {
    std::string filename = "shader.glsl";
    std::string host = "localhost";
    std::string model = "codellama:7b";
    
    if(argc >= 2)
        host = argv[1];
    if(argc >= 3)
        model = argv[2];
    if(argc >= 4)
        filename = argv[3];

    std::cout << "Ai Shader Generator..\n";
    std::cout << "(C) 2025 LostSideDead Software\n";
    std::cout.flush();
    
    std::ifstream fin("input.txt");
    std::ostringstream total;
    if(!fin.is_open()) {
        std::cerr << "Could not open input file\n";    
        return EXIT_FAILURE;
    }
    total << fin.rdbuf() << std::endl;
    fin.close();
    try {
        mx::ObjectRequest request(host, model, filename);
        request.setPrompt(total.str());
        std::string code = request.generateCode();
        if (code.empty()) {
            std::cout << "Failed to generate code\n";
            return EXIT_FAILURE;
        }
    } catch (const mx::ObjectRequestException &e) {
        std::cout << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return 0;
}