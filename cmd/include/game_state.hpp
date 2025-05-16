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

        void createList(const std::string& name) {
            if (lists.find(name) == lists.end()) {
                lists[name] = std::vector<std::string>();
            } else {
                throw StateException("List already exists: " + name);
            }
        }

        void filList(const std::string &name, const std::vector<std::string>& values) {
            auto it = lists.find(name);
            if (it != lists.end()) {
                it->second = values;
            } else {
                throw StateException("List not found: " + name);
            }
        }

        void initList(const std::string &name, const std::string &filln, size_t number) {
            auto it = lists.find(name);
            if (it != lists.end()) {
                if(!it->second.empty())
                    it->second.clear();    
                it->second.resize(number);
                for(size_t i = 0; i < number; ++i) {
                    it->second.at(i) = filln;
                }
            } else {
                throw StateException("List not found: " + filln);
            }
        }

        void setList(const std::string &name, size_t index, const std::string &value) {
            auto it = lists.find(name);
            if (it != lists.end()) {
                if (index < it->second.size()) {
                    it->second.at(index) = value;
                } else {
                    throw StateException("Index: " + std::to_string(index) + " out of bounds for list: " + name);
                }
            } else {
                throw StateException("List not found: " + name);
            }
        } 

        bool hasList(const std::string& name) const {
            return lists.find(name) != lists.end();
        }

        std::string getFromList(const std::string& name, int index) const {
            auto it = lists.find(name);
            if (it != lists.end()) {
                    if(index < 0 || index > static_cast<int>(it->second.size()-1)) {
                        throw StateException("Index of: " + name + " out of range: " + std::to_string(index));
                    }
                    return it->second.at(index);
            }
            throw StateException("List not found: " + name);
        }

        size_t getListLength(const std::string& name) const {
            auto it = lists.find(name);
            if (it != lists.end()) {
                return it->second.size();
            }
            throw StateException("List not found: " + name);
        }
        
        void addToList(const std::string& name, const std::string& value) {
            auto it = lists.find(name);
            if (it != lists.end()) {
                it->second.push_back(value);
                std::cout << "Added value: " << value << " to list: " << name << std::endl;
            } else {
                throw StateException("List not found: " + name);
            }

        }
        void removeFromList(const std::string& name, const std::string& value) {
            auto it = lists.find(name);
            if (it != lists.end()) {
                auto &list = it->second;
                auto pos = std::find(list.begin(), list.end(), value);
                if (pos != list.end()) {
                    list.erase(pos);
                } else {
                    throw StateException("Value not found in list: " + name);
                }
            } else {
                throw StateException("List not found: " + name);
            }
        }
        void clearList(const std::string& name) {
            auto it = lists.find(name);
            if (it != lists.end()) {
                it->second.clear();
            } else {
                throw StateException("List not found: " + name);
            }
        }
        void clearAllLists() {
            lists.clear();
        }

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
            auto listSorted = [&](std::ostream &o, auto &l) -> void {
                std::vector<std::pair<std::string, std::string>> lst;
                for (auto &f: l) {
                    lst.push_back(std::make_pair(f.first, f.second));
                }  
                std::sort(lst.begin(), lst.end());
                size_t maxNameLength = 0;
                for (auto &i : lst) {
                    maxNameLength = std::max(maxNameLength, i.first.length());
                }
                char currentLetter = '\0';
                int varIndex = 0;
                for (auto &i : lst) {
                    if (!i.first.empty() && i.first[0] != currentLetter) {
                        currentLetter = i.first[0];
                        if (varIndex > 0) {
                            o << "\n"; 
                        }
                        o << "  --- " << (char)std::toupper(currentLetter) << " ---\n";
                    }
                    o << "    " << std::setw(3) << std::right << varIndex++ << ": "
                    << std::setw(maxNameLength) << std::left << i.first << " = "
                    << "\"" << expandVariables(i.second) << "\"\n";
                }
                if (lst.empty()) {
                    o << "    (no variables defined)\n";
                }
            };
           auto listSorted_list = [&](std::ostream &o, auto &l) -> void {
                std::vector<std::pair<std::string, std::vector<std::string>>> lst;
                for(auto &f: l) {
                    lst.push_back(std::make_pair(f.first, f.second));
                }
                
                std::sort(lst.begin(), lst.end());
                size_t maxNameLength = 0;
                for(auto &i : lst) {
                    maxNameLength = std::max(maxNameLength, i.first.length());
                }
                
                int listIndex = 0;
                for(auto &i : lst) {
                    o << std::setfill(' ') << std::setw(3) << listIndex++ << ": " 
                    << std::setw(maxNameLength) << std::left << i.first << " {" << "\n";
                    int itemIndex = 0;
                    for (auto &v : i.second) {
                        o << "    " << std::setw(3) << std::right << itemIndex++ << ": " 
                        << v << "\n";
                    }
                    o << "}" << "\n\n";
                }
            };
            output << "Script Lists {\n";
            listSorted_list(output, lists);
            output << "}\n";
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
       std::unordered_map<std::string, std::vector<std::string>> lists;
    };

    GameState *getGameState();
}



#endif