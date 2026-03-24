
/**
 * @file argz.hpp
 * @brief Lightweight, header-only, template command-line argument parser.
 *
 * Supports both short options (@c -x) and long options (@c --name), with or
 * without values, for any string type that satisfies the StringType concept
 * (typically @c std::string or @c std::wstring).
 *
 * Typical usage:
 * @code
 * Argz<std::string> parser(argc, argv);
 * parser.addOptionSingle('h', "Show help")
 *       .addOptionSingleValue('o', "Output file");
 * Argument<std::string> arg;
 * int code;
 * while ((code = parser.proc(arg)) != -1) { ... }
 * @endcode
 *
 * A convenience wrapper proc_args() provides a pre-built parser that handles
 * the options common to all libmx2 applications (-p path, -r resolution, -f fullscreen, …).
 */

#ifndef _ARGZ_HPP_X
#define _ARGZ_HPP_X

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @concept StringType
 * @brief Models a string-like class with indexing, concatenation, length, and value_type.
 *
 * Both @c std::string and @c std::wstring satisfy this concept.
 * @tparam T The type to constrain.
 */
template <typename T>
concept StringType = std::is_class_v<T> && requires(T type) {
    type.length();
    type[0];
    type += type;
    type = type;
    typename T::value_type;
    typename T::size_type;
    { type.length() } -> std::same_as<typename T::size_type>;
    { type[0] } -> std::same_as<typename T::value_type &>;
    { type += T{} } -> std::same_as<T &>;
    { type = T{} } -> std::same_as<T &>;
};

/** @brief Discriminator for option kind used inside Argument. */
enum class ArgType { ARG_SINGLE,
                     ARG_SINGLE_VALUE,
                     ARG_DOUBLE,
                     ARG_DOUBLE_VALUE,
                     ARG_NONE };

/**
 * @struct Argument
 * @brief Describes a single parsed argument entry returned by Argz::proc().
 * @tparam String String type satisfying StringType.
 *
 * After a successful call to Argz::proc() the member @c arg_letter holds the
 * matched code (or @c '-' for bare positional arguments) and @c arg_value holds
 * any associated value string.
 */
template <StringType String>
struct Argument {
    String arg_name;  ///< Long option name (e.g. @c "output").
    int arg_letter;   ///< Short option code (e.g. @c 'o') or unique integer for long-only options.
    String arg_value; ///< Value string supplied after the option, if any.
    ArgType arg_type; ///< Which @c ArgType variant this argument represents.
    String desc;      ///< Human-readable description used in help output.
    ~Argument() = default;
    Argument() : arg_name{}, arg_letter{}, arg_value{}, arg_type{}, desc{} {}
    Argument(const Argument &a) : arg_name{a.arg_name}, arg_letter{a.arg_letter}, arg_value{a.arg_value}, arg_type{a.arg_type}, desc{a.desc} {}
    Argument &operator=(const Argument<String> &a) {
        arg_name = a.arg_name;
        arg_letter = a.arg_letter;
        arg_value = a.arg_value;
        arg_type = a.arg_type;
        desc = a.desc;
        return *this;
    }
    auto operator<=>(const Argument<String> &a) const { return (arg_letter <=> a.arg_letter); }
    bool operator==(const Argument<String> &a) const { return arg_letter == a.arg_letter; }
};

/**
 * @struct ArgumentData
 * @brief Holds the raw @c argv strings converted to the target @c String type.
 * @tparam String String type satisfying StringType.
 */
template <StringType String>
struct ArgumentData {
    std::vector<String> args; ///< Converted argv entries (argv[1] … argv[argc-1]).
    int argc;                 ///< Original argc value.
    ~ArgumentData() = default;
    ArgumentData() = default;
    ArgumentData(const ArgumentData<String> &a) : args{a.args}, argc{a.argc} {}
    ArgumentData &operator=(const ArgumentData<String> &a) {
        if (!args.empty()) {
            args.erase(args.begin(), args.end());
        }
        std::copy(a.args.begin(), a.args.end(), std::back_inserter(args));
        argc = a.argc;
        return *this;
    }
    ArgumentData(ArgumentData<String> &&a) : args{std::move(a.args)}, argc{a.argc} {}
    ArgumentData<String> &operator=(ArgumentData<String> &&a) {
        args = std::move(a.args);
        argc = a.argc;
        return *this;
    }
};

/**
 * @class ArgException
 * @brief Exception thrown by Argz::proc() on unrecognised or malformed options.
 * @tparam String String type satisfying StringType.
 */
template <StringType String>
class ArgException {
  public:
    ArgException() = default;
    ArgException(const String &s) : value{s} {}
    String text() const { return value; }

  private:
    String value;
};

/**
 * @class Argz
 * @brief Template command-line argument parser.
 * @tparam String String type satisfying StringType (usually @c std::string or @c std::wstring).
 *
 * Options are registered with addOption*() before parsing.
 * Call proc() in a loop until it returns @c -1 to iterate over all arguments.
 */
template <StringType String>
class Argz {
  public:
    ~Argz() = default;
    Argz() = default;

    /**
     * @brief Construct and immediately ingest argv.
     * @param argc Argument count from main().
     * @param argv Argument vector from main().
     */
    Argz(int argc, char **argv) { initArgs(argc, argv); }
    Argz(const Argz<String> &a) : arg_data{a.arg_data}, arg_info{a.arg_info}, index{a.index}, cindex{a.cindex} {}

    Argz<String> &operator=(const Argz<String> &a) {
        arg_data = a.arg_data;
        if (!arg_info.empty()) {
            arg_info.erase(arg_info.begin(), arg_info.end());
        }
        for (const auto &i : a.arg_info) {
            arg_info[i.first] = i.second;
        }
        index = a.index;
        cindex = a.cindex;
        return *this;
    }

    Argz(Argz<String> &&a) : arg_data{std::move(a.arg_data)}, arg_info{std::move(a.arg_info)}, index{a.index}, cindex{a.cindex} {}

    Argz<String> &operator=(Argz<String> &&a) {
        arg_data = std::move(a.arg_data);
        arg_info = std::move(a.arg_info);
        index = a.index;
        cindex = a.cindex;
        return *this;
    }

    /**
     * @brief Ingest argc/argv, converting to the target String type.
     * @param argc Argument count.
     * @param argv Argument vector.
     * @return Reference to @c *this for method chaining.
     */
    Argz<String> &initArgs(int argc, char **argv) {
        arg_data.argc = argc;
        if constexpr (std::is_same<typename String::value_type, char>::value) {
            for (int i = 1; i < argc; ++i) {
                const char *a = argv[i];
                arg_data.args.push_back(a);
            }
            reset();
            return *this;
        }
        if constexpr (std::is_same<typename String::value_type, wchar_t>::value) {
            for (int i = 1; i < argc; ++i) {
                const char *a = argv[i];
                String data;
                for (size_t z = 0; a[z] != 0; ++z) {
                    data += static_cast<typename String::value_type>(a[z]);
                }
                arg_data.args.push_back(data);
            }
            reset();
            return *this;
        }
        reset();
        return *this;
    }

    /** @brief Reset the internal parse cursor to the beginning of argv. */
    void reset() {
        index = 0;
        cindex = 1;
    }

    /**
     * @brief Register a flag-only short option (e.g. @c -v, @c -h).
     * @param c           Short option character code.
     * @param description Help text.
     * @return Reference to @c *this for chaining.
     */
    Argz<String> &addOptionSingle(const int &c, const String &description) {
        Argument<String> a{};
        a.arg_letter = c;
        a.arg_type = ArgType::ARG_SINGLE;
        a.desc = description;
        arg_info[c] = a;
        return *this;
    }

    /**
     * @brief Register a short option that requires a value argument (e.g. @c -o file).
     * @param c           Short option character code.
     * @param description Help text.
     * @return Reference to @c *this for chaining.
     */
    Argz<String> &addOptionSingleValue(const int &c, const String &description) {
        Argument<String> a{};
        a.arg_letter = c;
        a.arg_type = ArgType::ARG_SINGLE_VALUE;
        a.desc = description;
        arg_info[c] = a;
        return *this;
    }

    /**
     * @brief Register a flag-only long option (e.g. @c --verbose).
     * @param code        Unique integer code associated with this option.
     * @param value       Long option name string (without @c --).
     * @param description Help text.
     * @return Reference to @c *this for chaining.
     */
    Argz<String> &addOptionDouble(const int &code, const String &value, const String &description) {
        Argument<String> a{};
        a.arg_letter = code;
        a.arg_type = ArgType::ARG_DOUBLE;
        a.desc = description;
        a.arg_name = value;
        arg_info[code] = a;
        return *this;
    }

    /**
     * @brief Register a long option that requires a value argument (e.g. @c --output file).
     * @param code        Unique integer code.
     * @param value       Long option name string.
     * @param description Help text.
     * @return Reference to @c *this for chaining.
     */
    Argz<String> &addOptionDoubleValue(const int &code, const String &value, const String &description) {
        Argument<String> a{};
        a.arg_letter = code;
        a.arg_type = ArgType::ARG_DOUBLE_VALUE;
        a.desc = description;
        a.arg_name = value;
        arg_info[code] = a;
        return *this;
    }

    /**
     * @brief Look up the integer code registered for a long option name.
     * @param value Long option name (without @c --).
     * @return The registered code, or @c -1 if not found.
     */
    int lookUpCode(const String &value) {
        for (const auto &i : arg_info) {
            if (i.second.arg_name == value) {
                return i.second.arg_letter;
            }
        }
        return -1;
    }

    /**
     * @brief Advance the cursor and parse the next argument.
     * @param a Output: filled with the matched option details.
     * @return The option code on success; @c '-' for a positional argument; @c -1 when exhausted.
     * @throws ArgException<String> On unrecognised options or missing values.
     */
    int proc(Argument<String> &a) {
        if (index < static_cast<int>(arg_data.args.size())) {
            const String &type{arg_data.args[index]};
            if (type.length() > 3 && type[0] == '-' && type[1] == '-') {
                String name{};
                for (size_t z = 2; z < type.length(); ++z)
                    name += type[z];
                int code = lookUpCode(name);
                if (code != -1) {
                    auto pos = arg_info.find(code);
                    if (pos != arg_info.end()) {
                        if (pos->second.arg_type == ArgType::ARG_DOUBLE) {
                            a = pos->second;
                            a.arg_name = name;
                            index++;
                            return code;
                        } else {
                            a = pos->second;
                            a.arg_name = name;
                            if (++index < static_cast<int>(arg_data.args.size()) && arg_data.args[index][0] != '-') {
                                a.arg_name = name;
                                a.arg_value = arg_data.args[index];
                                index++;
                                return code;
                            } else {
                                if constexpr (std::is_same<typename String::value_type, char>::value) {
                                    throw ArgException<String>("Expected Value");
                                }
                            }
                        }
                    }
                } else {
                    if constexpr (std::is_same<typename String::value_type, char>::value) {
                        String value = "Error argument: ";
                        value += name;
                        value += " switch not found";
                        throw ArgException<String>(value);
                    }
                    if constexpr (std::is_same<typename String::value_type, wchar_t>::value) {
                        String value = L"Error argument: ";
                        value += name;
                        value += L" switch not found";
                        throw ArgException<String>(value);
                    }
                }
            } else if (type.length() == 1 && type[0] == '-') {
                if constexpr (std::is_same<typename String::value_type, char>::value) {
                    throw ArgException<String>("Expected Value found -");
                }
                if constexpr (std::is_same<typename String::value_type, wchar_t>::value) {
                    throw ArgException<String>(L"Expected Value found -");
                }
            } else if (type.length() > 1 && (type[0] == '-')) {
                const int c{type[cindex]};
                const auto pos{arg_info.find(c)};
                cindex++;
                if (cindex >= static_cast<int>(type.length())) {
                    cindex = 1;
                    index++;
                }
                String name_val{};
                name_val += static_cast<typename String::value_type>(c);
                if (pos != arg_info.end()) {
                    if (pos->second.arg_type == ArgType::ARG_SINGLE) {
                        a = pos->second;
                        a.arg_name = name_val;
                        return c;
                    } else if (pos->second.arg_type == ArgType::ARG_SINGLE_VALUE) {
                        if (index < static_cast<int>(arg_data.args.size())) {
                            const String &s{arg_data.args[index]};
                            if (s.length() > 1 && s[0] == '-' && !(s[1] >= '0' && s[1] <= '9')) {
                                if constexpr (std::is_same<typename String::value_type, char>::value) {
                                    throw ArgException<String>("Expected value");
                                }
                                if constexpr (std::is_same<typename String::value_type, wchar_t>::value) {
                                    throw ArgException<String>(L"Expected value");
                                }
                            } else if (s.length() == 1 && s[0] == '-') {
                                if constexpr (std::is_same<typename String::value_type, char>::value) {
                                    throw ArgException<String>("Expected Value found -");
                                }
                                if constexpr (std::is_same<typename String::value_type, wchar_t>::value) {
                                    throw ArgException<String>(L"Expected Value found -");
                                }
                            }
                            if (s.length() > 0) {
                                a = pos->second;
                                a.arg_value = s;
                                a.arg_name = name_val;
                                index++;
                                return c;
                            }
                        } else {
                            if constexpr (std::is_same<typename String::value_type, char>::value) {
                                throw ArgException<String>("Expected Value");
                            }
                            if constexpr (std::is_same<typename String::value_type, wchar_t>::value) {
                                throw ArgException<String>(L"Expected Value");
                            }
                        }
                    } else {
                        if constexpr (std::is_same<typename String::value_type, char>::value) {
                            throw ArgException<String>("Invalid switch not found!");
                        }
                        if constexpr (std::is_same<typename String::value_type, wchar_t>::value) {
                            throw ArgException<String>(L"Invalid switch not found!");
                        }
                    }
                } else {
                    if constexpr (std::is_same<typename String::value_type, char>::value) {
                        String value;
                        value = "Error argument ";
                        value += static_cast<typename String::value_type>(c);
                        value += " switch not found.";
                        throw ArgException<String>(value);
                    }
                    if constexpr (std::is_same<typename String::value_type, wchar_t>::value) {
                        String value;
                        value = L"Error argument ";
                        value += static_cast<typename String::value_type>(c);
                        value += L" switch not found.";
                        throw ArgException<String>(value);
                    }
                }
            } else {
                a = Argument<String>();
                a.arg_name = String{};
                a.arg_type = ArgType::ARG_NONE;
                a.arg_name = a.arg_value = arg_data.args.at(index);
                index++;
                return '-';
            }
        }
        return -1;
    }

    /**
     * @brief Print all registered options to any ostream-like object.
     * @tparam T Output stream type (e.g. @c std::ostream, @c std::wostream).
     * @param cout Destination stream.
     */
    template <typename T>
    void help(T &cout) {
        using char_type = typename std::decay<decltype(*std::declval<T>().rdbuf())>::type::char_type;
        std::vector<Argument<String>> v;
        std::vector<Argument<String>> v2;
        for (const auto &i : arg_info) {
            if (i.second.arg_type == ArgType::ARG_SINGLE || i.second.arg_type == ArgType::ARG_SINGLE_VALUE)
                v.push_back(i.second);
            else if (i.second.arg_type == ArgType::ARG_DOUBLE || i.second.arg_type == ArgType::ARG_DOUBLE_VALUE)
                v2.push_back(i.second);
        }
        std::ranges::sort(v);
        std::ranges::sort(v2);
        std::vector<Argument<String>> farg;
        farg.reserve(v.size() + v2.size());
        std::copy(v.begin(), v.end(), std::back_inserter(farg));
        std::copy(v2.begin(), v2.end(), std::back_inserter(farg));
        for (auto a = farg.begin(); a != farg.end(); ++a) {
            if (a->arg_type == ArgType::ARG_SINGLE || a->arg_type == ArgType::ARG_SINGLE_VALUE) {
                if constexpr (std::is_same<char_type, char>::value) {
                    String item;
                    item += static_cast<char_type>(a->arg_letter);
                    cout << "-" << std::setfill(' ') << std::setw(9) << std::left << item << "\t";
                    cout << std::setfill(' ') << std::left << std::setw(10) << a->desc;
                    cout << '\n';
                } else if constexpr (std::is_same<char_type, wchar_t>::value) {
                    String item;
                    item += static_cast<char_type>(a->arg_letter);
                    cout << L"-" << std::setfill(L' ') << std::setw(9) << std::left << item << L"\t";
                    cout << std::setfill(L' ') << std::setw(10) << a->desc;
                    cout << L'\n';
                }
            } else {
                if constexpr (std::is_same<char_type, char>::value) {
                    cout << "--";
                    cout << std::setfill(' ') << std::left << std::setw(10) << a->arg_name;
                    cout << "\t";
                    cout << std::setw(10) << a->desc;
                    cout << '\n';
                } else if constexpr (std::is_same<char_type, wchar_t>::value) {
                    cout << L"--";
                    cout << std::setfill(L' ') << std::left << std::setw(10) << a->arg_name;
                    cout << L"\t";
                    cout << std::setw(10) << std::left << a->desc;
                    cout << L'\n';
                }
            }
        }
    }
    /** @return Number of argv entries consumed so far. */
    const size_t count() const { return index; }

  protected:
    ArgumentData<String> arg_data;
    std::unordered_map<int, Argument<String>> arg_info;

  private:
    int index = 0, cindex = 1;
};

/**
 * @struct Arguments
 * @brief Plain data structure returned by proc_args() with all common libmx2 CLI options.
 */
struct Arguments {
    int width;              ///< Viewport width in pixels (default: 1280).
    int height;             ///< Viewport height in pixels (default: 720).
    std::string path;       ///< Asset search path (default: ".").
    bool fullscreen;        ///< Whether fullscreen mode was requested.
    std::string filename;   ///< Optional input filename (@c --filename).
    std::string texture;    ///< Optional texture file path (@c --texture).
    std::string shaderPath; ///< Optional SPV shader folder path (@c -S / @c --shader-path).
};

/**
 * @brief Parse standard libmx2 command-line options from main()'s argv.
 *
 * Registers and processes the following options:
 * | Flag | Long form          | Description                                  |
 * |------|--------------------|----------------------------------------------|
 * | -h   |                    | Print help and exit                          |
 * | -p   | --path             | Asset directory path                         |
 * | -r   | --resolution       | Resolution as WxH (e.g. 1920x1080)           |
 * | -f   | --fullscreen       | Enable fullscreen                            |
 * |      | --filename         | Input filename                               |
 * |      | --texture          | Texture file                                 |
 * | -S   | --shader-path      | SPV shader folder (must contain index.txt)   |
 *
 * @param argc Reference to argc from main().
 * @param argv argv from main().
 * @return Populated Arguments struct; on parse error, returns a default-valued struct.
 */
inline Arguments proc_args(int &argc, char **argv) {
    Arguments args;
    Argz<std::string> parser(argc, argv);
    parser.addOptionSingle('h', "Display help message")
        .addOptionSingleValue('p', "assets path")
        .addOptionDoubleValue('P', "path", "assets path")
        .addOptionSingleValue('r', "Resolution WidthxHeight")
        .addOptionDoubleValue('R', "resolution", "Resolution WidthxHeight")
        .addOptionSingle('f', "fullscreen")
        .addOptionDouble('F', "fullscreen", "fullscreen")
        .addOptionDoubleValue(256, "filename", "input filename")
        .addOptionDoubleValue(257, "texture", "texture file (.png or .tex)")
        .addOptionSingleValue('S', "shader SPV folder path (contains index.txt)")
        .addOptionDoubleValue(258, "shader-path", "shader SPV folder path (contains index.txt)");

    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 1280, th = 720;
    bool fullscreen = false;
    std::string filename;
    std::string texture;
    std::string shaderPath;
    try {
        while ((value = parser.proc(arg)) != -1) {
            switch (value) {
            case 256:
                filename = arg.arg_value;
                break;
            case 257:
                texture = arg.arg_value;
                break;
            case 'S':
            case 258:
                shaderPath = arg.arg_value;
                break;
            case 'h':
            case 'v':
                parser.help(std::cout);
                exit(EXIT_SUCCESS);
                break;
            case 'p':
            case 'P':
                path = arg.arg_value;
                break;
            case 'r':
            case 'R': {
                auto pos = arg.arg_value.find("x");
                if (pos == std::string::npos) {
                    std::cerr << "Error invalid resolution use WidthxHeight\n";
                    std::cerr.flush();
                    exit(EXIT_FAILURE);
                }
                std::string left, right;
                left = arg.arg_value.substr(0, pos);
                right = arg.arg_value.substr(pos + 1);
                tw = atoi(left.c_str());
                th = atoi(right.c_str());
            } break;
            case 'f':
            case 'F':
                fullscreen = true;
                break;
            }
        }
    } catch (const ArgException<std::string> &e) {
        std::cerr << "mx: Argument Exception" << e.text() << std::endl;
        args.width = 1280;
        args.height = 720;
        args.path = ".";
        args.fullscreen = false;
        return args;
    }
    if (path.empty()) {
        std::cerr << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
    args.width = tw;
    args.height = th;
    args.path = path;
    args.fullscreen = fullscreen;
    args.filename = filename;
    args.texture = texture;
    args.shaderPath = shaderPath;
    return args;
}

#endif