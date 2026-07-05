#pragma once
#include <string>
#include <cstdint>

namespace IkigaiScript {

// Decode the first UTF-8 code point starting at str[pos].
// Returns the code point and advances pos past the decoded bytes.
// Returns U+FFFD for invalid sequences.
inline char32_t utf8DecodeOne(const std::string& str, size_t& pos) {
    if (pos >= str.size()) return 0;
    unsigned char c0 = (unsigned char)str[pos];
    if (c0 < 0x80) { ++pos; return c0; }
    if ((c0 & 0xE0) == 0xC0 && pos + 1 < str.size()) {
        unsigned char c1 = (unsigned char)str[pos + 1];
        if ((c1 & 0xC0) == 0x80) {
            pos += 2;
            return ((char32_t)(c0 & 0x1F) << 6) | (c1 & 0x3F);
        }
    }
    if ((c0 & 0xF0) == 0xE0 && pos + 2 < str.size()) {
        unsigned char c1 = (unsigned char)str[pos + 1];
        unsigned char c2 = (unsigned char)str[pos + 2];
        if ((c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80) {
            pos += 3;
            return ((char32_t)(c0 & 0x0F) << 12) | ((char32_t)(c1 & 0x3F) << 6) | (c2 & 0x3F);
        }
    }
    if ((c0 & 0xF8) == 0xF0 && pos + 3 < str.size()) {
        unsigned char c1 = (unsigned char)str[pos + 1];
        unsigned char c2 = (unsigned char)str[pos + 2];
        unsigned char c3 = (unsigned char)str[pos + 3];
        if ((c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80) {
            pos += 4;
            return ((char32_t)(c0 & 0x07) << 18) | ((char32_t)(c1 & 0x3F) << 12)
                 | ((char32_t)(c2 & 0x3F) << 6) | (c3 & 0x3F);
        }
    }
    ++pos; return 0xFFFD;  // replacement character
}

// Encode a code point to UTF-8 and append to result.
inline void utf8Encode(char32_t cp, std::string& result) {
    if (cp < 0x80) { result += (char)cp; }
    else if (cp < 0x800) {
        result += (char)(0xC0 | (cp >> 6));
        result += (char)(0x80 | (cp & 0x3F));
    }
    else if (cp < 0x10000) {
        result += (char)(0xE0 | (cp >> 12));
        result += (char)(0x80 | ((cp >> 6) & 0x3F));
        result += (char)(0x80 | (cp & 0x3F));
    }
    else {
        result += (char)(0xF0 | (cp >> 18));
        result += (char)(0x80 | ((cp >> 12) & 0x3F));
        result += (char)(0x80 | ((cp >> 6) & 0x3F));
        result += (char)(0x80 | (cp & 0x3F));
    }
}

// Count the number of Unicode code points in a UTF-8 string.
inline size_t utf8Length(const std::string& s) {
    size_t count = 0;
    size_t pos = 0;
    while (pos < s.size()) {
        utf8DecodeOne(s, pos);
        ++count;
    }
    return count;
}

// Get the Nth code point (0-indexed) from a UTF-8 string.
// Returns 0 if index is out of range.
inline char32_t utf8At(const std::string& s, size_t n) {
    size_t pos = 0, count = 0;
    while (pos < s.size()) {
        char32_t cp = utf8DecodeOne(s, pos);
        if (count == n) return cp;
        ++count;
    }
    return 0;
}

// Extract a substring by code point indices [start, end] inclusive.
inline std::string utf8Slice(const std::string& s, size_t start, size_t end) {
    std::string result;
    size_t pos = 0, count = 0;
    while (pos < s.size() && count <= end) {
        size_t prev = pos;
        utf8DecodeOne(s, pos);
        if (count >= start) result += s.substr(prev, pos - prev);
        ++count;
    }
    return result;
}

// Decode first UTF-8 code point from a raw string literal content (after quotes stripped).
// Handles \n \t \\ \" \' \u{XXXX} escapes.
inline char32_t decodeCharLiteral(const std::string& val) {
    if (val.empty()) return 0;
    // Handle \u{XXXX} or \uXXXX unicode escapes
    if (val.size() >= 2 && val[0] == '\\') {
        if (val[1] == 'u') {
            size_t start = 2;
            bool hasBrace = start < val.size() && val[start] == '{';
            if (hasBrace) ++start;
            size_t endPos = start;
            while (endPos < val.size() && isxdigit((unsigned char)val[endPos])) ++endPos;
            std::string hexStr = val.substr(start, endPos - start);
            if (!hexStr.empty()) {
                unsigned long cp = 0;
                for (char c : hexStr) {
                    cp = cp * 16 + (isdigit((unsigned char)c) ? c - '0' : tolower(c) - 'a' + 10);
                }
                return (char32_t)cp;
            }
        }
        if (val[1] == 'n') return '\n';
        if (val[1] == 't') return '\t';
        if (val[1] == '\\') return '\\';
        if (val[1] == '\'') return '\'';
        if (val[1] == '"') return '"';
        return (char32_t)(unsigned char)val[1];
    }
    // Decode the first UTF-8 code point
    size_t pos = 0;
    return utf8DecodeOne(val, pos);
}

} // namespace IkigaiScript
