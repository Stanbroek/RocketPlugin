#pragma once
#include <string>
#include <vector>
#include <algorithm>


/*
 *  Wide string parser equivalents.
 */

// Checks if the given string starts with given string.
inline bool string_starts_with(const std::wstring& wstr, const std::wstring& begin)
{
    return wstr.compare(0, begin.length(), begin) == 0;
}


// Splits the string by delimiter.
inline size_t split(const std::wstring& wstr, std::vector<std::wstring>& wstrs, const wchar_t delimiter)
{
    size_t pos = wstr.find(delimiter);
    size_t initialPos = 0;
    wstrs.clear();

    // Decompose statement.
    while (pos != std::string::npos) {
        wstrs.push_back(wstr.substr(initialPos, pos - initialPos));
        initialPos = pos + 1;

        pos = wstr.find(delimiter, initialPos);
    }

    // Add the last one.
    wstrs.push_back(wstr.substr(initialPos, std::min(pos, wstr.size()) - initialPos));
	
    return wstrs.size();
}


// Trim from start.
inline std::wstring& ltrim(std::wstring& wstr) {
    wstr.erase(wstr.begin(), std::find_if(wstr.begin(), wstr.end(), [](const int wc) {
	    return !std::isspace(wc);
    }));

    return wstr;
}


// Trim from end.
inline std::wstring& rtrim(std::wstring& wstr) {
    wstr.erase(std::find_if(wstr.rbegin(), wstr.rend(), [](const int wc) {
	    return !std::isspace(wc);
    }).base(), wstr.end());

    return wstr;
}


// Trim from both ends.
inline std::wstring& trim(std::wstring& wstr) {
    return ltrim(rtrim(wstr));
}


// Replaces the first occurrence with the other string.
inline bool replace(std::wstring& wstr, const std::wstring& from, const std::wstring& to) {
    const size_t start_pos = wstr.find(from);
    if (start_pos == std::string::npos) {
        return false;
    }
    wstr.replace(start_pos, from.length(), to);

    return true;
}


// Replaces all occurrences with the other string.
inline void replace_all(std::wstring& wstr, const std::wstring& from, const std::wstring& to) {
    size_t start_pos = wstr.find(from);
    while (start_pos != std::string::npos) {
        wstr.replace(start_pos, from.length(), to);
        start_pos = wstr.find(from);
    }
}
