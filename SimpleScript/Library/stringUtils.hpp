#pragma once

#include <vector>
#include <string>
#include <charconv>
#include <functional>

namespace IkigaiScript {
    inline double toDouble(const std::string& token) {
        double x = 0;
        std::from_chars(token.data() + (token[0] == '+' ? 1 : 0), token.data() + token.size(), x);
        return x;
    }

    inline double toDouble(std::string_view token) {
        double x = 0;
        std::from_chars(token.data() + (token[0] == '+' ? 1 : 0), token.data() + token.size(), x);
        return x;
    }

    // Parse numeric literals including hex (0x/0X) and binary (0b/0B).
    inline double parseNumericLiteral(std::string_view token) {
        if (token.size() >= 2 && token[0] == '0') {
            if (token[1] == 'x' || token[1] == 'X') {
                unsigned long long v = 0;
                std::from_chars(token.data() + 2, token.data() + token.size(), v, 16);
                return static_cast<double>(v);
            }
            if (token[1] == 'b' || token[1] == 'B') {
                unsigned long long v = 0;
                std::from_chars(token.data() + 2, token.data() + token.size(), v, 2);
                return static_cast<double>(v);
            }
        }
        return toDouble(token);
    }

    inline double parseNumericLiteral(const std::string& token) {
        return parseNumericLiteral(std::string_view(token));
    }
    
    template<typename T, typename C>
    inline bool contains(const C& container, const T& element) {
        return std::find(container.begin(), container.end(), element) != container.end();
    }

    inline bool endswith(const std::string& v, const std::string& end) {
        if (end.size() > v.size()) {
            return false;
        }
        return equal(end.rbegin(), end.rend(), v.rbegin());
    }

    inline bool startswith(const std::string& v, const std::string& start) {
        if (start.size() > v.size()) {
            return false;
        }
        return equal(start.begin(), start.end(), v.begin());
    }

    inline bool contain(const std::string& v, const std::string& sub) {
        return v.find(sub) != std::string::npos;
    }
    inline bool contain(std::string_view v, const std::string& sub) {
        return (v.find(sub) != std::string::npos);
    }

    inline std::vector<std::string> split(const std::string& input, const std::string& pattern) {
        std::vector<std::string> ret;
        if (input.empty()) return ret;
        size_t pos = 0;
        size_t lpos = 0;
        auto dlen = pattern.length();
        while ((pos = input.find(pattern, lpos)) != std::string::npos) {
            ret.push_back(input.substr(lpos, pos - lpos));
            lpos = pos + dlen;
        }
        if (lpos < input.length()) {
            ret.push_back(input.substr(lpos, input.length()));
        }
        return ret;
    }

    inline std::vector<std::string_view> split(std::string_view input, char delimiter) {
        std::vector<std::string_view> ret;
		if (input.empty()) return ret;
		size_t pos = 0;
		size_t lpos = 0;
		while ((pos = input.find(delimiter, lpos)) != std::string::npos) {
			ret.push_back(input.substr(lpos, pos - lpos));
			lpos = pos + 1;
		}
		ret.push_back(input.substr(lpos, input.length()));
		return ret;
	}

    inline void replaceWhitespaceLiterals(std::string& input) {
        size_t pos = 0;
        while ((pos = input.find('\\', pos)) != std::string::npos) {
            if (pos + 1 >= input.size()) break;
            switch (input[pos + 1]) {
            case 't':  input.replace(pos, 2, "\t"); break;
            case 'n':  input.replace(pos, 2, "\n"); break;
            case 'r':  input.replace(pos, 2, "\r"); break;
            case '\\': input.replace(pos, 2, "\\"); break;
            case '\'': input.replace(pos, 2, "'"); break;
            case '"':  input.replace(pos, 2, "\""); break;
            case 'u': {
                // \u{XXXX} or \uXXXX
                size_t start = pos + 2;
                bool hasBrace = start < input.size() && input[start] == '{';
                if (hasBrace) ++start;
                size_t end = start;
                while (end < input.size() && isxdigit((unsigned char)input[end])) ++end;
                size_t escLen = end - pos + (hasBrace ? 1 : 0);  // include closing }
                if (hasBrace && end < input.size() && input[end] == '}') end++;
                std::string hex = input.substr(start, end - start - (hasBrace ? 1 : 0));
                if (!hex.empty()) {
                    unsigned long cp = 0;
                    for (char c : hex) cp = cp * 16 + (isdigit((unsigned char)c) ? c - '0' : tolower((unsigned char)c) - 'a' + 10);
                    std::string encoded;
                    size_t replaceEnd = hasBrace ? end : start + hex.size();
                    if (cp < 0x80) encoded += (char)cp;
                    else if (cp < 0x800) { encoded += (char)(0xC0 | (cp >> 6)); encoded += (char)(0x80 | (cp & 0x3F)); }
                    else if (cp < 0x10000) { encoded += (char)(0xE0 | (cp >> 12)); encoded += (char)(0x80 | ((cp >> 6) & 0x3F)); encoded += (char)(0x80 | (cp & 0x3F)); }
                    else { encoded += (char)(0xF0 | (cp >> 18)); encoded += (char)(0x80 | ((cp >> 12) & 0x3F)); encoded += (char)(0x80 | ((cp >> 6) & 0x3F)); encoded += (char)(0x80 | (cp & 0x3F)); }
                    input.replace(pos, replaceEnd - pos, encoded);
                    pos += encoded.size();
                } else { ++pos; }
                continue;
            }
            default: ++pos; continue;
            }
            // After replacement, single char — no advance needed since we replaced in-place
        }
    }


    inline static const char* ws = " \t\n\r\f\v";
    
    inline std::string& rtrim(std::string& s, const char* t = ws) {
        s.erase(s.find_last_not_of(t) + 1);
        return s;
    }
    
    inline std::string& ltrim(std::string& s, const char* t = ws) {
        s.erase(0, s.find_first_not_of(t));
        return s;
    }

    inline std::string& trim(std::string& s, const char* t = ws) {
        return ltrim(rtrim(s, t), t);
    }

    inline std::string_view ltrim(std::string_view str) {
        const auto pos(str.find_first_not_of(ws));
        str.remove_prefix(std::min(pos, str.length()));
        return str;
    }

    inline std::string_view rtrim(std::string_view str) {
        const auto pos(str.find_last_not_of(ws));
        str.remove_suffix(std::min(str.length() - pos - 1, str.length()));
        return str;
    }

    inline std::string_view trim(std::string_view str) {
        return ltrim(rtrim(str));
    }
}
