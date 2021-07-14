#pragma once
#include "stackwalker.h"
#include "se_exception.h"


#define COMMA_ ,
#define RBRACKET_ }
#define LBRACKET_ {
#define RPARENT_ )
#define LPARENT_ (

#define SET_SE_TRANSLATOR					\
	//static std::once_flag _se_translator_f;	\
	//std::call_once(_se_translator_f, []() {	\
	//	SE_Exception::SetSETranslator();	\
	//})

#define LOG_EXCEPTIONS(desc, msg, trace)	\
	LOG("=== Critical error: ===\n{}\n{}\n\n{}", desc, msg, trace)

#define CATCH_EXCEPTIONS(desc)																					\
	catch (const std::exception& e) {																			\
		try {																									\
			LOG_EXCEPTIONS(desc, e.what(), StackWalkerBM().DumpCallStack());									\
		}																										\
		catch (...) { CRITICAL_LOG("Failed to get callstack {}", e.what()); }									\
	}																											\
	catch (const SE_Exception& e) {																				\
		try {																									\
			LOG_EXCEPTIONS(desc, e.getSeMessage(), StackWalkerBM().DumpCallStack(e.GetExceptionPointers()));	\
		}																										\
		catch (...) { CRITICAL_LOG("Failed to get callstack {}", e.getSeMessage()); }							\
	}																											\
	catch (...) {																								\
		try {																									\
			LOG_EXCEPTIONS(desc, "Unknown exception", StackWalkerBM().DumpCallStack());							\
		}																										\
		catch (...) { CRITICAL_LOG("Failed to get callstack"); }												\
	}

#define TRY_CATCH_EXCEPTIONS(body, desc)	\
	try {									\
		body								\
		return;								\
	}										\
	CATCH_EXCEPTIONS(desc)

inline void Nothing(void*) {}

inline int LogCallStack(const std::string& desc, PEXCEPTION_POINTERS pExceptionPointers)
{
	if (pExceptionPointers == nullptr || pExceptionPointers->ExceptionRecord == nullptr) {
		CRITICAL_LOG("could not get the exception record");
		return EXCEPTION_CONTINUE_SEARCH;
	}

	const DWORD exceptionCode = pExceptionPointers->ExceptionRecord->ExceptionCode;
	std::string exceptionMessage = GetExceptionMessage(exceptionCode);
	const std::string exceptionCallStack = StackWalkerBM().DumpCallStack(pExceptionPointers);

	if (exceptionMessage.empty()) {
		try {
			// MSVC exception ptr hack.
			// https://github.com/microsoft/STL/blob/68b344c9dcda6ddf1a8fca112cb9033f9e50e787/stl/src/excptptr.cpp#L476
			auto exceptionRecord = std::shared_ptr<const EXCEPTION_RECORD>(pExceptionPointers->ExceptionRecord, Nothing);
			__ExceptionPtrRethrow(&exceptionRecord);
		}
		catch (const std::exception& e) {
			exceptionMessage = e.what();
		}
		catch (const std::string& str) {
			exceptionMessage = str;
		}
		catch (const char* buf) {
			exceptionMessage = buf ? std::string(buf, strnlen_s(buf, 1024)) : "empty exception";
		}
		catch (...) {
			exceptionMessage = SE_Exception::FormatSeMessage(exceptionCode);
		}
	}

	LOG_EXCEPTIONS(desc, exceptionMessage, exceptionCallStack);

	if (exceptionCode == DBG_CONTROL_C ||
		exceptionCode == EXCEPTION_STACK_OVERFLOW ||
		exceptionCode == EXCEPTION_NONCONTINUABLE_EXCEPTION ||
		exceptionCode == EXCEPTION_BREAKPOINT) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

template<typename Func>
inline bool GuardedFunction(const std::string& desc, Func lambda)
{
	__try {
		lambda();
		return true;
	}
	__except (LogCallStack(desc, GetExceptionInformation())) {
		MessageBox(NULL,
			"Addional info should be provided in the F6 console.\n"
			"A crashdump should be located in \"%appdata%\\bakkesmod\\bakkesmod\\crashes\".\n"
			"You can continue to use BakkesMod and Rocket Plugin.",
			"Rocket Plugin Error", MB_OK | MB_ICONERROR);
	}

	return false;
}

#define TRY_EXCEPT(desc, body, args)							\
	if (GuardedFunction(desc, [&]() { body(args); })) return;

#define CATCH_FUNCTION(func, body, args, desc, before, after)	\
	func														\
	{															\
		before													\
		/*TRY_CATCH_EXCEPTIONS(body(args), #func ", " desc)*/		\
		TRY_EXCEPT(#func ", " desc, body, args)					\
		after													\
	}

#define CATCH_OVERRIDE_FUNCTION(funcOverride, func, body, args, desc, before, after)	\
	CATCH_FUNCTION(funcOverride, body, args, desc, before, after)						\
	func

#define CATCH_ONLOAD													\
	CATCH_OVERRIDE_FUNCTION(void onLoad(), void OnLoad(), OnLoad,		\
		, "Game thread exception:", SET_SE_TRANSLATOR;,					\
		cvarManager->executeCommand("plugin unload RocketPlugin");)

#define CATCH_ONUNLOAD														\
	CATCH_OVERRIDE_FUNCTION(void onUnload(), void OnUnload(), OnUnload,		\
		, "Game thread exception:", , )

#define CATCH_RENDER														\
	CATCH_OVERRIDE_FUNCTION(void Render(), void OnRender(), OnRender,		\
		, "Rendering thread exception:", SET_SE_TRANSLATOR;,				\
		cvarManager->executeCommand("closemenu " + GetMenuName());			\
		ImGui::GetStateStorage()->Clear();									\
		ImGui::NewFrame();)

#define CATCH_RENDER_SETTINGS																		\
	CATCH_OVERRIDE_FUNCTION(void RenderSettings(), void OnRenderSettings(), OnRenderSettings,		\
		, "Rendering thread exception:", SET_SE_TRANSLATOR;,										\
		cvarManager->executeCommand("closemenu settings");)

#define CATCH_HOOK_EVENT																											\
	CATCH_FUNCTION(void HookEvent(const std::string& eventName, std::function<void(const std::string& eventName)> callback) const,	\
		callback, _eventName, + quote(eventName) + ", Game thread exception:",														\
		gameWrapper->HookEvent LPARENT_ eventName COMMA_ [=](const std::string& _eventName) LBRACKET_, RBRACKET_ RPARENT_;)

#define CATCH_HOOK_EVENT_POST																											\
	CATCH_FUNCTION(void HookEventPost(const std::string& eventName, std::function<void(const std::string& eventName)> callback) const,	\
		callback, _eventName, + quote(eventName) + ", Game thread exception:",															\
		gameWrapper->HookEvent LPARENT_ eventName COMMA_ [=](const std::string& _eventName) LBRACKET_, RBRACKET_ RPARENT_;)

#define CATCH_HOOK_EVENT_WITH_CALLER																																																										\
	CATCH_FUNCTION(template<typename T  COMMA_ typename std::enable_if<std::is_base_of<ObjectWrapper  COMMA_ T>::value>::type* = nullptr> void HookEventWithCaller(const std::string& eventName, const std::function<void(T, void*, const std::string&)>& callback) const,	\
		callback, caller COMMA_ params COMMA_ _eventName, + quote(eventName) + ", Game thread exception:",																																									\
		gameWrapper->HookEventWithCaller<T> LPARENT_ eventName COMMA_ [=](T caller, void* params, const std::string& _eventName) LBRACKET_, RBRACKET_ RPARENT_;)

#define CATCH_HOOK_EVENT_WITH_CALLER_POST																																																										\
	CATCH_FUNCTION(template<typename T  COMMA_ typename std::enable_if<std::is_base_of<ObjectWrapper  COMMA_ T>::value>::type* = nullptr> void HookEventWithCallerPost(const std::string& eventName, const std::function<void(T, void*, const std::string&)>& callback) const,	\
		callback, caller COMMA_ params COMMA_ _eventName, + quote(eventName) + ", Game thread exception:",																																										\
		gameWrapper->HookEventWithCallerPost<T> LPARENT_ eventName COMMA_ [=](T caller, void* params, const std::string& _eventName) LBRACKET_, RBRACKET_ RPARENT_;)

#define CATCH_SET_TIMEOUT																					\
	CATCH_FUNCTION(void SetTimeout(const std::function<void(GameWrapper*)>& theLambda, float time) const,	\
		theLambda, gw, "Game thread exception:",															\
		gameWrapper->SetTimeout LPARENT_ [=](GameWrapper* gw) LBRACKET_, RBRACKET_ COMMA_ time RPARENT_;)

#define CATCH_EXECUTE																		\
	CATCH_FUNCTION(void Execute(const std::function<void(GameWrapper*)>& theLambda) const,	\
		theLambda, gw, "Game thread exception:",											\
		gameWrapper->Execute LPARENT_ [=](GameWrapper* gw) LBRACKET_, RBRACKET_ RPARENT_;)

#define CATCH_REGISTER_NOTIFIER																																										\
	CATCH_FUNCTION(void RegisterNotifier(const std::string& cvar, const std::function<void(std::vector<std::string>)>& notifier, const std::string& description, unsigned char permissions) const,	\
		notifier, arguments, + quote(cvar) + ", Game thread exception:",																															\
		cvarManager->registerNotifier LPARENT_ cvar COMMA_ [=](const std::vector<std::string>& arguments) LBRACKET_, RBRACKET_ COMMA_ description COMMA_ permissions RPARENT_;)

#define CATCH_DEFAULT_BM_FUNCTIONS		\
	CATCH_ONLOAD;						\
	CATCH_ONUNLOAD;						\
	CATCH_HOOK_EVENT					\
	CATCH_HOOK_EVENT_POST				\
	CATCH_HOOK_EVENT_WITH_CALLER		\
	CATCH_HOOK_EVENT_WITH_CALLER_POST	\
	CATCH_SET_TIMEOUT					\
	CATCH_EXECUTE						\
	CATCH_REGISTER_NOTIFIER
