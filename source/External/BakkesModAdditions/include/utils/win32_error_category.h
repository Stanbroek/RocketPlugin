/// From https://gist.github.com/bbolli/710010adb309d5063111889530237d6d
#pragma once
#include <Windows.h>
#include <system_error>

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
    class win32_error_category final : public std::error_category
    {
    public:
        /// Return a short descriptive name for the category.
        _NODISCARD char const* name() const noexcept override
        {
            return "Win32Error";
        }

        /// Return what each error code means in text.
        _NODISCARD std::string message(const int errVal) const override
        {
            const DWORD exceptionCode = errVal;
            std::string exceptionMessage = getExceptionMessage(exceptionCode);
            if (!exceptionMessage.empty()) {
                return exceptionMessage;
            }

            char errorBuffer[256] = "";
            const DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM;
            DWORD len = FormatMessageA(flags, LoadLibrary(TEXT("NTDLL.DLL")),
                exceptionCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                errorBuffer, sizeof errorBuffer, NULL);
            if (len == 0) {
                return "N/A (0x" + to_hex(exceptionCode) + ")";
            }
            // trim trailing newline
            while (len > 0 && isspace(errorBuffer[len - 1])) {
                --len;
            }

            return std::string(errorBuffer, len) + " (0x" + to_hex(exceptionCode) + ")";
        }

    private:
        static std::string getExceptionMessage(const DWORD exceptionCode)
        {
            // From https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-exception_record#members
            switch (exceptionCode) {
                case EXCEPTION_ACCESS_VIOLATION:
                    return "The thread tried to read from or write to a virtual address for which it does not have the appropriate access.";
                case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
                    return "The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.";
                case EXCEPTION_BREAKPOINT:
                    return "A breakpoint was encountered.";
                case EXCEPTION_DATATYPE_MISALIGNMENT:
                    return "The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16 - bit values must be aligned on 2 - byte boundaries; 32 - bit values on 4 - byte boundaries, and so on.";
                case EXCEPTION_FLT_DENORMAL_OPERAND:
                    return "One of the operands in a floating - point operation is denormal. A denormal value is one that is too small to represent as a standard floating - point value.";
                case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                    return "The thread tried to divide a floating - point value by a floating - point divisor of zero.";
                case EXCEPTION_FLT_INEXACT_RESULT:
                    return "The result of a floating - point operation cannot be represented exactly as a decimal fraction.";
                case EXCEPTION_FLT_INVALID_OPERATION:
                    return "This exception represents any floating - point exception not included in this list.";
                case EXCEPTION_FLT_OVERFLOW:
                    return "The exponent of a floating - point operation is greater than the magnitude allowed by the corresponding type.";
                case EXCEPTION_FLT_STACK_CHECK:
                    return "The stack overflowed or underflowed as the result of a floating - point operation.";
                case EXCEPTION_FLT_UNDERFLOW:
                    return "The exponent of a floating - point operation is less than the magnitude allowed by the corresponding type.";
                case EXCEPTION_ILLEGAL_INSTRUCTION:
                    return "The thread tried to execute an invalid instruction.";
                case EXCEPTION_IN_PAGE_ERROR:
                    return "The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.";
                case EXCEPTION_INT_DIVIDE_BY_ZERO:
                    return "The thread tried to divide an integer value by an integer divisor of zero.";
                case EXCEPTION_INT_OVERFLOW:
                    return "The result of an integer operation caused a carry out of the most significant bit of the result.";
                case EXCEPTION_INVALID_DISPOSITION:
                    return "An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high - level language such as C should never encounter this exception.";
                case EXCEPTION_NONCONTINUABLE_EXCEPTION:
                    return "The thread tried to continue execution after a noncontinuable exception occurred.";
                case EXCEPTION_PRIV_INSTRUCTION:
                    return "The thread tried to execute an instruction whose operation is not allowed in the current machine mode.";
                case EXCEPTION_SINGLE_STEP:
                    return "A trace trap or other single - instruction mechanism signaled that one instruction has been executed.";
                case EXCEPTION_STACK_OVERFLOW:
                    return "The thread used up its stack.";
                default:
                    break;
            }

            return std::string();
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


namespace std
{
    // Tell the C++ 11 STL metaprogramming that win32_error_code
    // is registered with the standard error code system.
    template <>
    struct is_error_code_enum<win32_error_code> : std::true_type {};
}


class win32_exception : public std::exception {
public:
    explicit win32_exception(const DWORD e) : std::exception(make_win32_error_code(e).message().c_str()) {}
};


/// Create an exception from the last Windows error.
inline win32_exception make_win32_error_exception()
{
    return win32_exception(GetLastError());
}
