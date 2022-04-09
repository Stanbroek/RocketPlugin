#pragma once
#include <Windows.h>

#include "win32_error_category.h"


/// Create an error_code from the last Winsock error.
inline std::error_code make_winsock_error_code()
{
    return make_win32_error_code(WSAGetLastError());
}


/// Create an exception from the last Winsock error.
inline win32_exception make_winsock_error_exception()
{
    return win32_exception(WSAGetLastError());
}
