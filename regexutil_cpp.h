/**
 * regexutil.h
 * Simplified C++ regex functions
 * 
 * Copyright (c) 2026 HGS Team
 * Author: Administrator
 * 
 * Regular expression syntax is really weird. You'd only know if you've used it.
 * 
 * Usage examples:
 *   match("hello123", "\\d+");                    // Check if contains digits
 *   search("hello123", "\\d+");                    // Find first match
 *   searchAll("a1 b2 c3", "\\w+");                  // Find all words
 *   replace("hello world", "world", "C++");         // Replace text
 *   replaceAll("a1b2c3", "\\d", "X");               // Replace all digits with X
 *   isValid("\\d+");                                 // Check if regex is valid
 *   split("a,b,c", ",");                             // Split string by comma
 * 
 * Note: In C++ strings, backslashes need escaping: "\\d" for digits, "\\w" for word chars
 *       Or use raw string: R"(\d+)" is same as "\\d+"
 */

#ifndef REGEXUTIL_H
#define REGEXUTIL_H

#include <regex>
#include <string>
#include <vector>

// -------------------- Basic Matching --------------------

inline bool match(const std::string& str, const std::string& pattern) {
    // Check if string contains pattern (anywhere)
    try {
        std::regex re(pattern);
        return std::regex_search(str, re);
    } catch (...) {
        return false;
    }
}

inline bool matchExact(const std::string& str, const std::string& pattern) {
    // Check if entire string matches pattern exactly
    try {
        std::regex re(pattern);
        return std::regex_match(str, re);
    } catch (...) {
        return false;
    }
}

// -------------------- Searching --------------------

inline std::string search(const std::string& str, const std::string& pattern) {
    // Find first match, return empty string if not found
    try {
        std::regex re(pattern);
        std::smatch match;
        if (std::regex_search(str, match, re)) {
            return match.str();
        }
    } catch (...) {
        // ignore
    }
    return "";
}

inline std::vector<std::string> searchAll(const std::string& str, const std::string& pattern) {
    // Find all matches, return vector
    std::vector<std::string> results;
    try {
        std::regex re(pattern);
        std::sregex_iterator it(str.begin(), str.end(), re);
        std::sregex_iterator end;
        
        for (; it != end; ++it) {
            results.push_back(it->str());
        }
    } catch (...) {
        // ignore
    }
    return results;
}

// -------------------- Capturing Groups --------------------

inline std::vector<std::string> capture(const std::string& str, const std::string& pattern) {
    // Capture groups from first match
    std::vector<std::string> groups;
    try {
        std::regex re(pattern);
        std::smatch match;
        if (std::regex_search(str, match, re)) {
            for (size_t i = 1; i < match.size(); i++) {
                groups.push_back(match[i].str());
            }
        }
    } catch (...) {
        // ignore
    }
    return groups;
}

inline std::vector<std::vector<std::string>> captureAll(const std::string& str, const std::string& pattern) {
    // Capture groups from all matches
    std::vector<std::vector<std::string>> allGroups;
    try {
        std::regex re(pattern);
        std::sregex_iterator it(str.begin(), str.end(), re);
        std::sregex_iterator end;
        
        for (; it != end; ++it) {
            std::vector<std::string> groups;
            for (size_t i = 1; i < it->size(); i++) {
                groups.push_back((*it)[i].str());
            }
            allGroups.push_back(groups);
        }
    } catch (...) {
        // ignore
    }
    return allGroups;
}

// -------------------- Replacement --------------------

inline std::string replace(const std::string& str, const std::string& pattern, 
                           const std::string& replacement) {
    // Replace first occurrence only
    try {
        std::regex re(pattern);
        return std::regex_replace(str, re, replacement, 
                                   std::regex_constants::format_first_only);
    } catch (...) {
        return str;
    }
}

inline std::string replaceAll(const std::string& str, const std::string& pattern, 
                              const std::string& replacement) {
    // Replace all occurrences
    try {
        std::regex re(pattern);
        return std::regex_replace(str, re, replacement);
    } catch (...) {
        return str;
    }
}

// -------------------- Validation --------------------

inline bool isValid(const std::string& pattern) {
    // Check if regex pattern is valid
    try {
        std::regex re(pattern);
        return true;
    } catch (const std::regex_error&) {
        return false;
    }
}

// -------------------- Splitting --------------------

inline std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
    // Split string by delimiter pattern
    std::vector<std::string> tokens;
    try {
        std::regex re(delimiter);
        std::sregex_token_iterator it(str.begin(), str.end(), re, -1);
        std::sregex_token_iterator end;
        
        for (; it != end; ++it) {
            tokens.push_back(*it);
        }
    } catch (...) {
        tokens.push_back(str);  // return original string on error
    }
    return tokens;
}

// -------------------- Common Patterns (Predefined) --------------------

inline bool isEmail(const std::string& str) {
    // Simple email validation
    return matchExact(str, R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
}

inline bool isPhone(const std::string& str) {
    // Simple US phone validation (123-456-7890)
    return matchExact(str, R"(\d{3}-\d{3}-\d{4})");
}

inline bool isNumber(const std::string& str) {
    // Check if string contains only digits
    return matchExact(str, R"(\d+)");
}

inline bool isWord(const std::string& str) {
    // Check if string contains only word chars (letters, digits, underscore)
    return matchExact(str, R"(\w+)");
}

inline bool isHex(const std::string& str) {
    // Check if string is hex color (3 or 6 hex digits, optional #)
    return matchExact(str, R"(#?([a-fA-F0-9]{6}|[a-fA-F0-9]{3}))");
}

inline bool isIP(const std::string& str) {
    // Simple IPv4 validation
    return matchExact(str, 
        R"((\d{1,3}\.){3}\d{1,3})");
}

inline bool isURL(const std::string& str) {
    // Simple URL validation
    return matchExact(str, 
        R"(https?://[^\s]+)");
}

// -------------------- Extraction Helpers --------------------

inline std::vector<std::string> extractEmails(const std::string& str) {
    // Extract all email addresses from string
    return searchAll(str, R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
}

inline std::vector<std::string> extractNumbers(const std::string& str) {
    // Extract all numbers from string
    return searchAll(str, R"(\d+)");
}

inline std::vector<std::string> extractWords(const std::string& str) {
    // Extract all words from string
    return searchAll(str, R"(\w+)");
}

inline std::vector<std::string> extractUrls(const std::string& str) {
    // Extract all URLs from string
    return searchAll(str, R"(https?://[^\s]+)");
}

// -------------------- Raw String Helper --------------------

inline std::string raw(const std::string& pattern) {
    // Helper to note: In C++11+, use R"(pattern)" directly
    // This function just returns the string, but the name reminds you to use raw strings
    return pattern;
}

// -------------------- Pattern Builder (Fluent Style) --------------------

class RegexBuilder {
private:
    std::string m_pattern;
    
public:
    RegexBuilder() {}
    
    RegexBuilder& start() {
        m_pattern += "^";
        return *this;
    }
    
    RegexBuilder& end() {
        m_pattern += "$";
        return *this;
    }
    
    RegexBuilder& digits(int min = 1, int max = -1) {
        m_pattern += "\\d";
        if (max > 0) {
            m_pattern += "{" + std::to_string(min) + "," + std::to_string(max) + "}";
        } else if (min > 1) {
            m_pattern += "{" + std::to_string(min) + ",}";
        } else if (min == 0) {
            m_pattern += "*";
        } else {
            m_pattern += "+";
        }
        return *this;
    }
    
    RegexBuilder& letters() {
        m_pattern += "[a-zA-Z]+";
        return *this;
    }
    
    RegexBuilder& whitespace() {
        m_pattern += "\\s+";
        return *this;
    }
    
    RegexBuilder& exactly(const std::string& text) {
        m_pattern += text;
        return *this;
    }
    
    RegexBuilder& optional(const std::string& group) {
        m_pattern += "(" + group + ")?";
        return *this;
    }
    
    RegexBuilder& either(const std::string& a, const std::string& b) {
        m_pattern += "(" + a + "|" + b + ")";
        return *this;
    }
    
    std::string build() {
        return m_pattern;
    }
};

#endif
