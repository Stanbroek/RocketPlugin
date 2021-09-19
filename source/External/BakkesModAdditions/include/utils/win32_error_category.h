/// From https://gist.github.com/bbolli/710010adb309d5063111889530237d6d
#pragma once
#include <system_error>
#include <WinSock2.h>

#include "utils/stringify.h"


/// Wrap the Win32 error code so we have a distinct type.
struct win32_error_code
{
    explicit win32_error_code(const DWORD e) noexcept : Error(e) {}
    DWORD Error;
};


namespace detail
{
    /// The Win32 error code category.
    class win32_error_category : public std::error_category
    {
    public:
        /// Return a short descriptive name for the category.
        _NODISCARD char const* name() const noexcept final
        {
            return "Win32Error";
        }

        /// Return what each error code means in text.
        _NODISCARD std::string message(int errVal) const final
        {
            char error[1024];
            DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                static_cast<DWORD>(errVal), 0, error, sizeof error, NULL);
            if (len == 0) {
                return "N/A";
            }
            // trim trailing newline
            while (len && (error[len - 1] == '\r' || error[len - 1] == '\n')) {
                --len;
            }

            return std::string(error, len) + " (0x" + to_hex(errVal) + ")";
        }
    };
}


/// Return a static instance of the custom category.
static detail::win32_error_category const& win32_error_category()
{
    static detail::win32_error_category c;
    return c;
}


// Overload the global make_error_code() free function with our
// custom error. It will be found via ADL by the compiler if needed.
inline std::error_code make_error_code(const win32_error_code& winErrorCode)
{
    return { static_cast<int>(winErrorCode.Error), win32_error_category() };
}

/// Create an error_code from a Windows error code.
inline std::error_code make_win32_error_code(const DWORD errVal)
{
    return make_error_code(win32_error_code(errVal));
}


/// Create an error_code from the last Windows error.
inline std::error_code make_win32_error_code()
{
    return make_win32_error_code(GetLastError());
}


/// Create an error_code from the last Winsock error.
inline std::error_code make_winsock_error_code()
{
    return make_win32_error_code(WSAGetLastError());
}


namespace std
{
    // Tell the C++ 11 STL metaprogramming that win32_error_code
    // is registered with the standard error code system.
    template <>
    struct is_error_code_enum<win32_error_code> : std::true_type {};
}


class win32_error : public std::exception {
public:
    explicit win32_error(const DWORD e) : std::exception(make_win32_error_code(e).message().c_str()) {}
};


/// Create an exception from the last Windows error.
inline win32_error make_win32_error_exception()
{
    return win32_error(GetLastError());
}


/// Create an exception from the last Winsock error.
inline win32_error make_winsock_error_exception()
{
    return win32_error(WSAGetLastError());
}
