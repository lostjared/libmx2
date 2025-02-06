#include"cfg.hpp"
#include<algorithm>
#include"mx.hpp"
#include<sstream>


namespace mx {
    /*
    void ConfigFile::loadFile(const std::string &f) {
        std::ifstream in(f);
        if (!in.is_open()) {
            std::ostringstream stream;
            stream << "mx: Could not open configuration file: " <<  f << "\n";
            //throw mx::Exception(stream.str());
            return;

        }

        std::string line, currentSection;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }

            if (line.front() == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.size() - 2);
                continue;
            }

            size_t pos = line.find('=');
            if (pos != std::string::npos && !currentSection.empty()) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                Item item;
                item.key = key;
                item.value = value;

                values[currentSection][key] = item;
            }
        }

        in.close();
    }
    void ConfigFile::saveFile(const std::string &f2) {
        std::ofstream out(f2);
        if (!out.is_open()) {
            return;
        }

        std::vector<std::string> sectionNames;
        for (const auto &section : values) {
            sectionNames.push_back(section.first);
        }
        std::sort(sectionNames.begin(), sectionNames.end());

        for (const auto &section : sectionNames) {
            out << "[" << section << "]\n";

            std::vector<std::string> keys;
            for (const auto &pair : values[section]) {
                keys.push_back(pair.first);
            }
            std::sort(keys.begin(), keys.end());

            for (const auto &key : keys) {
                out << key << "=" << values[section][key].value << "\n";
            }

            out << "\n";
        }

        out.close();
    }

    Item ConfigFile::itemAtKey(const std::string &section, const std::string &key) {
        if(values[section].find(key) == values[section].end()) {
            std::ostringstream stream;
            stream << "mx: Could not find Key: " << key << " in config file.\n";;
            //throw mx::Exception(stream.str());
            return Item();
        }
        return values[section][key];
    }
        
    void ConfigFile::setItem(const std::string &section, const std::string &key, const std::string value) {
        values[section][key] = {key, value};
    }

    std::vector<std::string> ConfigFile::splitByComma(const std::string &str) {
        std::vector<std::string> result;
        size_t start = 0, end = 0;
        
        while ((end = str.find(',', start)) != std::string::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + 1;
        }
        
        result.push_back(str.substr(start));
        
        return result;
    }

    ConfigFile::ConfigFile(const std::string &filex) {
        loadFile(filex);
    }

    ConfigFile::~ConfigFile() {
        saveFile(filename);
    }
  */
}







