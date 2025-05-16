#ifndef _CMD_X_H_
#define _CMD_X_H_
#include<iostream>
#include<vector>
#include<string>
#include<sstream>
#include<fstream>
#include<unordered_map>
#include<set>
#include<regex>
#include"ast.hpp"


namespace cmd {

    extern std::vector<std::string> argv;

    int exitCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int echoCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output); 
    int catCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);
    int grepCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);
    int printCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int cdCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);
    int listCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);
    int sortCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);
    int findCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);
    int pwdCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int mkdirCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);  
    int cpCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);     
    int mvCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);     
    int touchCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);  
    int headCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);   
    int tailCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);   
    int wcCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);     
    int sedCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output);    
    int debugSet(const std::vector<Argument>& args, std::istream& input, std::ostream& output);      
    int debugGet(const std::vector<Argument>& args, std::istream& input, std::ostream& output);      
    int debugList(const std::vector<std::string>& args, std::istream& input, std::ostream& output);     
    int debugClear(const std::vector<Argument>& args, std::istream& input, std::ostream& output);   
    int debugClearAll(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int debugSearch(const std::vector<std::string>& args, std::istream& input, std::ostream& output); 
    int printfCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output);
    int dumpVariables(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int testCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output);
    int cmdCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output);
    int visualCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output);
    int atCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int lenCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int indexCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int strlenCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int strfindCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int strfindrCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int strtokCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int execCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int commandListCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int argvCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int externCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListAddCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListRemoveCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListGetCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListSetCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListClearCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListClearAllCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListExistsCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListInitCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListLenCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListTokens(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListSortCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListReverseCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListShuffleCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListCopyCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newListPopCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int concatListCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int newRandCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
    int exprCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output);
}
#endif