/// From https://docs.microsoft.com/en-us/cpp/cpp/exception-handling-differences?view=msvc-160#example---use-a-custom-translation-function
#pragma once
#include <eh.h>
#include <windows.h>

#include "utils/stringify.h"


class SE_Exception {
private:
	SE_Exception() {}
	unsigned int nSE;
	
public:
	SE_Exception( SE_Exception& e) : nSE(e.nSE) {}
	SE_Exception(unsigned int n) : nSE(n) {}
	~SE_Exception() {}

	static void trans_func(unsigned int u, _EXCEPTION_POINTERS*) { throw SE_Exception(u); }
	static void SetTranslator() { _set_se_translator(trans_func); }
	
	unsigned int getSeNumber() const { return nSE; }
	std::string getSeMessage() const
	{
		char error[1024];
		DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			static_cast<DWORD>(getSeNumber()), 0, error, sizeof error, NULL);
		if (len == 0) {
			return "N/A (" + to_hex(getSeNumber()) + ")";
		}
		// trim trailing newline
		while (len && (error[len - 1] == '\r' || error[len - 1] == '\n')) {
			--len;
		}

		return std::string(error, len) + " (" + to_hex(getSeNumber()) + ")";
	}
};