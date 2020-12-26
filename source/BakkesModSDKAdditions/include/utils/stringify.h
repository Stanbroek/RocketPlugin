#pragma once
#include <string>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>


template<typename T>
inline std::string to_hex(const T number)
{
    std::stringstream ss;
    std::make_unsigned_t<T> unsigned_number = number;
    ss << std::setw(sizeof number * 2) << std::setfill('0') << std::hex << static_cast<size_t>(unsigned_number);
    
    return "0x" + ss.str();
}


template<typename T>
inline std::string to_hex(const T* buffer, size_t size)
{
    std::string str = "[";
    for (size_t i = 0; i < size; i++) {
        str += " " + to_hex(buffer[i]);
        if (i + 1 < size) {
            str += ",";
        }
    }

    return str + " ]";
}


inline std::string get_addr(void* ptr)
{
    std::stringstream ss;
    ss << ptr;

    return "0x" + ss.str();
}


inline std::string to_lower(const std::string str)
{
    std::string str_cpy = str;
    std::transform(str_cpy.begin(), str_cpy.end(), str_cpy.begin(),
        [](unsigned char c) { return (unsigned char)std::tolower(c); });

    return str_cpy;
}

inline std::wstring to_lower(const std::wstring wstr)
{
    std::wstring wstr_cpy = wstr;
    std::transform(wstr_cpy.begin(), wstr_cpy.end(), wstr_cpy.begin(),
        [](wchar_t c) { return (wchar_t)std::tolower(c); });

    return wstr_cpy;
}


inline std::string to_upper(const std::string str)
{
    std::string str_cpy = str;
    std::transform(str_cpy.begin(), str_cpy.end(), str_cpy.begin(),
        [](unsigned char c) { return (unsigned char)std::toupper(c); });

    return str_cpy;
}

inline std::wstring to_upper(const std::wstring wstr)
{
    std::wstring wstr_cpy = wstr;
    std::transform(wstr_cpy.begin(), wstr_cpy.end(), wstr_cpy.begin(),
        [](wchar_t c) { return (wchar_t)std::toupper(c); });

    return wstr_cpy;
}


inline std::string quote(const std::string str)
{
    std::stringstream ss;
    ss << std::quoted(str);
    return ss.str();
}

inline std::wstring quote(const std::wstring wstr)
{
    std::wstringstream wss;
    wss << std::quoted(wstr);
    return wss.str();
}


inline std::string concat(const std::string str1, const std::string str2)
{
    return str1 + str2;
}

inline std::wstring concat(const std::wstring wstr1, const std::wstring wstr2)
{
    return wstr1 + wstr2;
}


inline std::string to_string(const std::string str)
{
    return str;
}

inline std::string to_string(const std::wstring wstr)
{
    std::filesystem::path p(wstr);

    return p.u8string();
}


inline std::wstring to_wstring(const std::wstring wstr)
{
    return wstr;
}

inline std::wstring to_wstring(const std::string str)
{
    std::filesystem::path p(str);

    return p.wstring();
}


struct case_insensitive_less {
    bool operator()(const std::string& _Left, const std::string& _Right) const {
        return to_lower(_Left) < to_lower(_Right);
    }

    bool operator()(const std::wstring& _Left, const std::wstring& _Right) const {
        return to_lower(_Left) < to_lower(_Right);
    }
};
