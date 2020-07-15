#pragma once
#include <string>
#include <vector>
#include <algorithm>


/*
 *  Widestring parser equivalents
 */

static inline bool string_starts_with(const std::wstring& value, const std::wstring& begin)
{
    return value.compare(0, begin.length(), begin) == 0;
}


static inline size_t split(const std::wstring& txt, std::vector<std::wstring>& strs, char ch)
{
    size_t pos = txt.find(ch);
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while (pos != std::string::npos) {
        strs.push_back(txt.substr(initialPos, pos - initialPos));
        initialPos = pos + 1;

        pos = txt.find(ch, initialPos);
    }

    // Add the last one
    strs.push_back(txt.substr(initialPos, std::min(pos, txt.size()) - initialPos));
    return strs.size();
}


// trim from start
static inline std::wstring& ltrim(std::wstring& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {return !std::isspace(c); }));

    return s;
}


// trim from end
static inline std::wstring& rtrim(std::wstring& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) {return !std::isspace(c); }).base(), s.end());

    return s;
}


// trim from both ends
static inline std::wstring& trim(std::wstring& s) {
    return ltrim(rtrim(s));
}


static inline bool replace(std::wstring& str, const std::wstring& from, const std::wstring& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}