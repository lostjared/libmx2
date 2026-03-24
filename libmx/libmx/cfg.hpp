/**
 * @file cfg.hpp
 * @brief INI-style configuration file reader/writer.
 *
 * ConfigFile parses simple section/key/value configuration files and
 * provides random-access retrieval and modification of settings.
 */
#ifndef CFG_H_X
#define CFG_H_X

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace mx {

    /**
     * @class Item
     * @brief A single key/value pair read from a configuration file.
     */
    class Item {
      public:
        std::string key;   ///< Configuration key name.
        std::string value; ///< Value associated with the key.
    };

    /**
     * @class ConfigFile
     * @brief Parses and manages an INI-style configuration file.
     *
     * Sections are denoted by [SectionName] headers.  Items within a section
     * are stored as key=value pairs.  The file can be loaded, queried,
     * modified in memory, and saved back to disk.
     */
    class ConfigFile {
        std::unordered_map<std::string, std::unordered_map<std::string, Item>> values;
        std::string filename;

      public:
        /** @brief Default constructor — no file loaded. */
        ConfigFile() = default;

        /**
         * @brief Open and parse a configuration file.
         * @param filex Path to the configuration file.
         */
        ConfigFile(const std::string &filex);

        /** @brief Destructor — closes any open file resources. */
        ~ConfigFile();

        ConfigFile(const ConfigFile &) = delete;
        ConfigFile &operator=(const ConfigFile &) = delete;
        ConfigFile(ConfigFile &&) = delete;
        ConfigFile &operator=(ConfigFile &&) = delete;

        /**
         * @brief Retrieve a configuration item by section and key.
         * @param section Section name (text inside [] brackets).
         * @param key     Key name within that section.
         * @return The matching Item, or a default-constructed one if not found.
         */
        Item itemAtKey(const std::string &section, const std::string &key);

        /**
         * @brief Insert or update a configuration item.
         * @param section Section name.
         * @param key     Key name.
         * @param value   New value string.
         */
        void setItem(const std::string &section, const std::string &key, const std::string value);

        /**
         * @brief Load (or reload) a configuration file from disk.
         * @param f Path to the file.
         */
        void loadFile(const std::string &f);

        /**
         * @brief Save the current configuration to a file.
         * @param f2 Destination file path.
         */
        void saveFile(const std::string &f2);

        /**
         * @brief Split a string on comma delimiters.
         * @param str Source string.
         * @return Vector of substrings between commas.
         */
        std::vector<std::string> splitByComma(const std::string &str);
    };
} // namespace mx

#endif