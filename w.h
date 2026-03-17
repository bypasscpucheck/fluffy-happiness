// remove_brackets_runtime.h
#ifndef REMOVE_BRACKETS_RUNTIME_H
#define REMOVE_BRACKETS_RUNTIME_H

#include <string>
#include <algorithm>

class IncludeProcessor {
public:
    // 移除尖括号
    static std::string removeAngleBrackets(const std::string& include) {
        std::string result = include;
        result.erase(std::remove(result.begin(), result.end(), '<'), result.end());
        result.erase(std::remove(result.begin(), result.end(), '>'), result.end());
        return result;
    }
    
    // 移除双引号
    static std::string removeQuotes(const std::string& include) {
        std::string result = include;
        result.erase(std::remove(result.begin(), result.end(), '"'), result.end());
        return result;
    }
    
    // 同时移除尖括号和双引号
    static std::string removeAll(const std::string& include) {
        std::string result = removeAngleBrackets(include);
        result = removeQuotes(result);
        return result;
    }
    
    // 提取文件名（移除路径和括号）
    static std::string extractFileName(const std::string& include) {
        std::string cleaned = removeAll(include);
        
        // 查找最后一个路径分隔符
        size_t lastSlash = cleaned.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            return cleaned.substr(lastSlash + 1);
        }
        
        return cleaned;
    }
};

#endif // REMOVE_BRACKETS_RUNTIME_H
