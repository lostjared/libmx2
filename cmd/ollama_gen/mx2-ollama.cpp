#include "mx2-ollama.hpp"

namespace mx {

   
    std::string ObjectRequest::unescape(const std::string &input) {
        std::string s = input;  
        
        size_t pos = 0;
        while ((pos = s.find("\\n", pos)) != std::string::npos) {
            s.replace(pos, 2, "\n");
            pos += 1;
        }
        
        pos = 0;
        while ((pos = s.find("\\t", pos)) != std::string::npos) {
            s.replace(pos, 2, "\t");
            pos += 1;
        }
        
        pos = 0;
        while ((pos = s.find("\\r", pos)) != std::string::npos) {
            s.replace(pos, 2, "\r");
            pos += 1;
        }

        std::regex unicode_regex(R"(\\u([0-9a-fA-F]{4}))");
        std::smatch match;
        while (std::regex_search(s, match, unicode_regex)) {
            int code = std::stoi(match[1].str(), nullptr, 16);
            char replacement = static_cast<char>(code);
            s.replace(match.position(), match.length(), 1, replacement);
        }
        return s;
    }

    size_t ObjectRequest::WriteCallback(void* contents, size_t size, size_t nmemb, ResponseData* data) {
        if (!data) return 0; 
        
        size_t total_size = size * nmemb;
        std::string chunk(static_cast<char*>(contents), total_size);
        
        std::istringstream stream(chunk);
        std::string line;
        
        while (std::getline(stream, line)) {
            static const std::regex re(R"REGEX("response"\s*:\s*"([^"]*)")REGEX");
            std::smatch m;
            
            if (std::regex_search(line, m, re)) {
                std::string unescaped = ObjectRequest::unescape(m[1].str());
                std::cout << unescaped;
                data->shader_stream << unescaped;
                std::cout.flush();
            }
        }
        
        data->response += chunk;
        return total_size;
    }

    std::string ObjectRequest::generateCode() {
        if (host.empty() || model.empty() || filename.empty() || prompt.empty()) {
            throw ObjectRequestException("Host, model, filename or prompt not set.");
        }

        
        std::ostringstream payload;
        payload << "{"
                << "\"model\":\"" << model << "\","
                << "\"prompt\":\"";
        
        
        std::string escaped_prompt;
        escaped_prompt.reserve(prompt.length() * 1.2); 
        
        for (char c : prompt) {
            switch (c) {
                case '"':  escaped_prompt += "\\\""; break;
                case '\\': escaped_prompt += "\\\\"; break;
                case '\n': escaped_prompt += "\\n"; break;
                case '\r': escaped_prompt += "\\r"; break;
                case '\t': escaped_prompt += "\\t"; break;
                default:   escaped_prompt += c; break;
            }
        }
        
        payload << escaped_prompt << "\"}";

        
        struct CurlRAII {
            CURL* curl;
            curl_slist* headers;
            
            CurlRAII() : curl(nullptr), headers(nullptr) {
                curl_global_init(CURL_GLOBAL_DEFAULT);
                curl = curl_easy_init();
                if (!curl) {
                    curl_global_cleanup();
                    throw ObjectRequestException("Failed to initialize curl");
                }
            }
            
            ~CurlRAII() {
                if (headers) {
                    curl_slist_free_all(headers);
                }
                if (curl) {
                    curl_easy_cleanup(curl);
                }
                curl_global_cleanup();
            }
            
            CurlRAII(const CurlRAII&) = delete;
            CurlRAII& operator=(const CurlRAII&) = delete;
        };

        CurlRAII curl_raii;
        ResponseData response_data;

        std::string url = "http://" + host + ":11434/api/generate";
        curl_easy_setopt(curl_raii.curl, CURLOPT_URL, url.c_str());
        
        std::string json_data = payload.str();
        curl_easy_setopt(curl_raii.curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl_raii.curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_data.length()));
        
        curl_raii.headers = curl_slist_append(curl_raii.headers, "Content-Type: application/json");
        if (!curl_raii.headers) {
            throw ObjectRequestException("Failed to create HTTP headers");
        }
        curl_easy_setopt(curl_raii.curl, CURLOPT_HTTPHEADER, curl_raii.headers);
        curl_easy_setopt(curl_raii.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_raii.curl, CURLOPT_WRITEDATA, &response_data);
        curl_easy_setopt(curl_raii.curl, CURLOPT_BUFFERSIZE, 1024L);
        curl_easy_setopt(curl_raii.curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl_raii.curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(curl_raii.curl, CURLOPT_CONNECTTIMEOUT, 60L);
        curl_easy_setopt(curl_raii.curl, CURLOPT_TIMEOUT, 300L); 
        CURLcode res = curl_easy_perform(curl_raii.curl);
        
        if (res != CURLE_OK) {
            throw ObjectRequestException("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
        }
        
 
        long response_code;
        curl_easy_getinfo(curl_raii.curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (response_code != 200) {
            throw ObjectRequestException("HTTP request failed with response code: " + std::to_string(response_code) + 
                                       "\nResponse: " + response_data.response);
        }
        
 
        return response_data.shader_stream.str();
    }
}