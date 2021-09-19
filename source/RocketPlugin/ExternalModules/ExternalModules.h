#pragma once


namespace ExternalModules
{
    inline std::vector<std::tuple<std::function<void()>>> ExternalModules;
    inline std::vector<std::tuple<std::string, std::string>> ExternalCVars;
    inline std::vector<std::tuple<std::string, std::function<void(const std::vector<std::string>&)>, std::string, unsigned char>> ExternalNotifiers;
    inline std::vector<std::tuple<std::string, std::function<void(ActorWrapper, const void*, const std::string&)>>> ExternalEventHooksPre;
    inline std::vector<std::tuple<std::string, std::function<void(ActorWrapper, const void*, const std::string&)>>> ExternalEventHooksPost;

    template<typename Ret, class... Args>
    static void RegisterModule(Ret(*func)(Args...), const Args&&... args)
    {
        ExternalModules.emplace_back([=]() -> void {
            func(std::forward<Args>(args)...);
        });
    }

    static void RegisterNotifier(const std::string& cvar, const std::function<void(const std::vector<std::string>&)>& notifier, const std::string& description, unsigned char permissions)
    {
        ExternalNotifiers.emplace_back(cvar, notifier, description, permissions);
    }

    static void RegisterHookEvent(const std::string& eventName, const std::function<void(const std::string&)>& callback)
    {
        ExternalEventHooksPre.emplace_back(eventName, [=](const ActorWrapper&, const void*, const std::string& eventName_) -> void {
            callback(eventName_);
        });
    }

    template<typename T, std::enable_if_t<std::is_base_of_v<ObjectWrapper, T>>* = nullptr>
    static void RegisterHookEventWithCaller(const std::string& eventName, const std::function<void(const T&, void*, const std::string&)>& callback)
    {
        ExternalEventHooksPre.emplace_back(eventName, [=](const ActorWrapper &caller, const void* params, const std::string& eventName_) -> void {
            callback(T(caller.memory_address), params, eventName_);
        });
    }

    static void RegisterHookEventPost(const std::string& eventName, const std::function<void(const std::string&)>& callback)
    {
        ExternalEventHooksPost.emplace_back(eventName, [=](const ActorWrapper &, const void* params, const std::string& eventName_) -> void {
            callback(eventName_);
        });
    }

    template<typename T, std::enable_if_t<std::is_base_of_v<ObjectWrapper, T>>* = nullptr>
    static void RegisterHookEventWithCallerPost(const std::string& eventName, const std::function<void(const T&, const void*, const std::string&)>& callback)
    {
        ExternalEventHooksPost.emplace_back(eventName, [=](const ActorWrapper &caller, const void* params, const std::string& eventName_) -> void {
            callback(T(caller.memory_address), params, eventName_);
        });
    }
}


#define CONCAT2(A, B) A ## B

#define CONCAT(A, B) CONCAT2(A, B)

#pragma section(".CRT$XCU", read)
#define RP_CTOR(fn)                         \
    static void fn(void);                   \
    __declspec(allocate(".CRT$XCU"))        \
        void(*CONCAT(fn, _))(void) = &fn;   \
    static void fn(void)

#define COUNTER(name) CONCAT(name, __COUNTER__)

#define RP_EXTERNAL_MODULE(...)                         \
    RP_CTOR(COUNTER(external_module_)) {                \
        ExternalModules::RegisterModule(__VA_ARGS__);   \
    }

#define RP_EXTERNAL_NOTIFIER                \
    RP_CTOR(COUNTER(external_notifier_)) {  \
        ExternalModules::RegisterNotifier

#define RP_EXTERNAL_HOOK_EVENT          \
    RP_CTOR(COUNTER(external_hook_)) {  \
        ExternalModules::RegisterHookEvent

#define RP_EXTERNAL_HOOK_EVENT_WITH_CALLER  \
    RP_CTOR(COUNTER(external_hook_)) {      \
        ExternalModules::RegisterHookEventWithCaller

#define RP_EXTERNAL_HOOK_EVENT_POST     \
    RP_CTOR(COUNTER(external_hook_)) {  \
        ExternalModules::RegisterHookEventPost

#define RP_EXTERNAL_HOOK_EVENT_WITH_CALLER_POST \
    RP_CTOR(COUNTER(external_hook_)) {          \
        ExternalModules::RegisterHookEventWithCallerPost

#ifdef DEBUG
    #define RP_EXTERNAL_DEBUG_MODULE                        RP_EXTERNAL_MODULE
    #define RP_EXTERNAL_DEBUG_NOTIFIER                      RP_EXTERNAL_NOTIFIER
    #define RP_EXTERNAL_DEBUG_HOOK_EVENT                    RP_EXTERNAL_HOOK_EVENT
    #define RP_EXTERNAL_DEBUG_HOOK_EVENT_WITH_CALLER        RP_EXTERNAL_HOOK_EVENT_WITH_CALLER
    #define RP_EXTERNAL_DEBUG_HOOK_EVENT_POST               RP_EXTERNAL_HOOK_EVENT_POST
    #define RP_EXTERNAL_DEBUG_HOOK_EVENT_WITH_CALLER_POST   RP_EXTERNAL_HOOK_EVENT_WITH_CALLER_POST
#else
    #define RP_EXTERNAL_DEBUG_MODULE(...)                       static void COUNTER(empty_func_)() { __noop; }
    #define RP_EXTERNAL_DEBUG_NOTIFIER(...)                     static void COUNTER(empty_func_)() { __noop
    #define RP_EXTERNAL_DEBUG_HOOK_EVENT(...)                   static void COUNTER(empty_func_)() { __noop
    #define RP_EXTERNAL_DEBUG_HOOK_EVENT_WITH_CALLER(...)       static void COUNTER(empty_func_)() { __noop
    #define RP_EXTERNAL_DEBUG_HOOK_EVENT_POST(...)              static void COUNTER(empty_func_)() { __noop
    #define RP_EXTERNAL_DEBUG_HOOK_EVENT_WITH_CALLER_POST(...)  static void COUNTER(empty_func_)() { __noop
#endif
