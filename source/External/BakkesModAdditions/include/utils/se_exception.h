/// From https://docs.microsoft.com/en-us/cpp/cpp/exception-handling-differences?view=msvc-160#example---use-a-custom-translation-function
#pragma once
#include <eh.h>
#include <string>
#include <Windows.h>

#include "stringify.h"


inline std::string GetExceptionMessage(const DWORD exceptionCode)
{
	switch (exceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:
			return "The thread tried to read from or write to a virtual address for which it does not have the appropriate access.";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			return "The thread tried to access an array element that is out of boundsand the underlying hardware supports bounds checking.";
		case EXCEPTION_BREAKPOINT:
			return "A breakpoint was encountered.";
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			return "The thread tried to read or write data that is misaligned on hardware that does not provide alignment.For example, 16 - bit values must be aligned on 2 - byte boundaries; 32 - bit values on 4 - byte boundaries, and so on.";
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			return "One of the operands in a floating - point operation is denormal.A denormal value is one that is too small to represent as a standard floating - point value.";
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
			return "The thread tried to access a page that was not present, and the system was unable to load the page.For example, this exception might occur if a network connection is lost while running a program over the network.";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			return "The thread tried to divide an integer value by an integer divisor of zero.";
		case EXCEPTION_INT_OVERFLOW:
			return "The result of an integer operation caused a carry out of the most significant bit of the result.";
		case EXCEPTION_INVALID_DISPOSITION:
			return "An exception handler returned an invalid disposition to the exception dispatcher.Programmers using a high - level language such as C should never encounter this exception.";
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


class SE_Exception {
public:
	SE_Exception(const SE_Exception& e) : nSE(e.nSE), pExp(e.pExp) {}
	SE_Exception(const unsigned int nSE_, EXCEPTION_POINTERS* pExp_) : nSE(nSE_), pExp(pExp_) {}
	~SE_Exception() = default;

	SE_Exception(SE_Exception&&) = default;
	SE_Exception& operator=(const SE_Exception&) = default;
	SE_Exception& operator=(SE_Exception&&) = default;

	static void se_trans_func(unsigned int nSE, EXCEPTION_POINTERS* pEP) { throw SE_Exception(nSE, pEP); }
	static _se_translator_function SetSETranslator() { return _set_se_translator(se_trans_func); }
	static std::string FormatSeMessage(const DWORD messageId)
	{
		char error[2048];
		DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
			LoadLibrary(TEXT("NTDLL.DLL")), static_cast<DWORD>(messageId),
			NULL, error, sizeof error, NULL);
		if (len == 0) {
			return "N/A (0x" + to_hex(messageId) + ")";
		}
		// trim trailing newline
		while (len && isspace(error[len - 1])) {
			--len;
		}

		return std::string(error, len) + " (0x" + to_hex(messageId) + ")";
	}

	unsigned int getSeNumber() const { return nSE; }
	std::string getSeMessage() const { return FormatSeMessage(getSeNumber()); }
	PEXCEPTION_POINTERS GetExceptionPointers() const { return pExp; }
	PCONTEXT GetContextRecord() const { return pExp->ContextRecord; }

private:
	SE_Exception() = default;
	
	unsigned int nSE = S_OK;
	EXCEPTION_POINTERS* pExp = nullptr;
};


class Scoped_SE_Translator
{
private:
	const _se_translator_function old_SE_translator;
public:
	Scoped_SE_Translator() noexcept
		: old_SE_translator{ SE_Exception::SetSETranslator() } {}
	~Scoped_SE_Translator() noexcept { _set_se_translator(old_SE_translator); }
};
