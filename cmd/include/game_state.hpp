#ifndef _GAME_STATE__
#define _GAME_STATE__

#include<string>
#include<iostream>
#include<iomanip>
#include<fstream>
#include<sstream>
#include<unordered_map>
#include<regex>
#include<set>


namespace state {

    class StateException {
        public:
            StateException(const std::string& message) : message(message) {}
            const char* what() const noexcept { return message.c_str(); }
        private: 
            std::string message;
    };

    class GameState {
    public:
        GameState() = default;
        ~GameState() = default;
        void setVariable(const std::string& name, const std::string& value) {
            variables[name] = value;
        }
        std::string getVariable(const std::string& name) const {
            auto it = variables.find(name);
            if (it != variables.end()) {
                return expandVariables(it->second);
            }
            throw StateException("Variable not found: " + name);
        }

        void listVariables(std::ostream& output) const {
            auto listSorted = [&](std::ostream  &o, auto &l) -> void {
                std::vector<std::pair<std::string, std::string>> lst;
                for(auto &f: l) {
                    lst.push_back(std::make_pair(f.first, f.second));

                }
                std::sort(lst.begin(), lst.end());
                for(auto &i : lst) {
                    o << std::setfill(' ') << std::setw(25) << i.first << "\t" << expandVariables(i.second)  << "\n";
                }
            };
            output << "Script Variables {\n";
            listSorted(output, variables);
            output << "}\n";
        }
        void clearVariables() {
            variables.clear();
        }
        void clearVariable(const std::string& name) {
            auto it = variables.find(name);
            if (it != variables.end()) {
                variables.erase(it);
            } else {
                throw StateException("Variable not found: " + name);
            }
        }

        void searchVariables(std::ostream &output, const std::string &pattern) {
            bool found = false;
            for(auto &v : variables) {
                std::regex re(pattern, std::regex::extended);
                if(std::regex_search(v.second, re)) {
                    found = true;
                    output << v.first << ": " << expandVariables(v.second) << "\n";
                } 
            }
            if(found == false) {
                output << "deubg: pattern not found.\n";
            }
        }

        bool dumpVariables(const std::string &filename) {
            std::fstream file;
            file.open(filename, std::ios::out);
            if(!file.is_open()) return false;

            for(auto &v : variables) {
                file << v.first << ": "  << v.second << "\n";
            }
            file.close();
            return true;
        }

        std::string expandVariables(const std::string& input, std::set<std::string> expandingVars = {}) const {
            std::string result = input;
            size_t pos = 0;
            while ((pos = result.find("%{", pos)) != std::string::npos) {
                size_t end = result.find("}", pos);
                if (end == std::string::npos) break;
                std::string varName = result.substr(pos + 2, end - pos - 2);
                
                if (expandingVars.find(varName) != expandingVars.end()) {
                    throw state::StateException("Circular reference detected for variable: " + varName);
                }
                
                auto it = variables.find(varName);
                if (it != variables.end()) {
                    std::set<std::string> newExpandingVars = expandingVars;
                    newExpandingVars.insert(varName);
                    std::string expandedValue = expandVariables(it->second, newExpandingVars);
                    result.replace(pos, end - pos + 1, expandedValue);
                    pos += expandedValue.length();
                } else {
                    result.replace(pos, end - pos + 1, "");
                }
            }
            return result; 
        }        
    protected:
       std::unordered_map<std::string, std::string> variables;
    };

    GameState *getGameState();
}



#endif