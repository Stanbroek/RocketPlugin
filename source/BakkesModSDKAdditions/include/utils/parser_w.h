#pragma once
#include <string>
#include <vector>
#include <algorithm>


/*
 *  Widestring parser equivalents
 */

inline bool string_starts_with(const std::wstring& wstr, const std::wstring& begin)
{
    return wstr.compare(0, begin.length(), begin) == 0;
}


inline size_t split(const std::wstring& wstr, std::vector<std::wstring>& wstrs, wchar_t wc)
{
    size_t pos = wstr.find(wc);
    size_t initialPos = 0;
    wstrs.clear();

    // Decompose statement
    while (pos != std::string::npos) {
        wstrs.push_back(wstr.substr(initialPos, pos - initialPos));
        initialPos = pos + 1;

        pos = wstr.find(wc, initialPos);
    }

    // Add the last one
    wstrs.push_back(wstr.substr(initialPos, std::min(pos, wstr.size()) - initialPos));
    return wstrs.size();
}


// trim from start
inline std::wstring& ltrim(std::wstring& wstr) {
    wstr.erase(wstr.begin(), std::find_if(wstr.begin(), wstr.end(), [](int wc) {return !std::isspace(wc); }));

    return wstr;
}


// trim from end
inline std::wstring& rtrim(std::wstring& wstr) {
    wstr.erase(std::find_if(wstr.rbegin(), wstr.rend(), [](int wc) {return !std::isspace(wc); }).base(), wstr.end());

    return wstr;
}


// trim from both ends
inline std::wstring& trim(std::wstring& wstr) {
    return ltrim(rtrim(wstr));
}


inline bool replace(std::wstring& wstr, const std::wstring& from, const std::wstring& to) {
    size_t start_pos = wstr.find(from);
    if (start_pos == std::string::npos)
        return false;
    wstr.replace(start_pos, from.length(), to);

    return true;
}


inline void replace_all(std::wstring& wstr, const std::wstring& from, const std::wstring& to) {
    size_t start_pos = wstr.find(from);
    while (start_pos != std::string::npos) {
        wstr.replace(start_pos, from.length(), to);
        start_pos = wstr.find(from);
    }
}
