#pragma once
#include "RocketPlugin.h"
#include "Modules/RocketPluginModule.h"


class RocketGameMode : public RocketPluginModule
{
public:
    RocketGameMode() = default;
    virtual ~RocketGameMode() = default;
    RocketGameMode(const RocketGameMode&) = delete;
    RocketGameMode(RocketGameMode&&) = delete;
    RocketGameMode& operator=(const RocketGameMode&) = delete;
    RocketGameMode& operator=(RocketGameMode&&) = delete;

    /* RocketGameMode event hook functions */
    using EventCallback = RocketPlugin::EventCallback;

    template<typename Caller>
    void HookPre(Caller caller, void* params, const std::string& eventName) const
    {
        const auto funcIt = Outer()->callbacksPre.find(eventName);
        if (funcIt != Outer()->callbacksPre.end()) {
            for (auto& [type, func] : funcIt->second) {
                func(static_cast<void*>(&caller), params, eventName);
            }
        }
    }

    void HookEvent(const std::string& eventName, const std::function<void(std::string)>& callback) const
    {
        BM_TRACE_LOG("{:s} hooking {:s}", quote(typeIdx->name()), quote(eventName));

        const auto it = Outer()->callbacksPre.find(eventName);
        if (it == Outer()->callbacksPre.end()) {
            Outer()->callbacksPre[eventName] = std::unordered_map<std::type_index, EventCallback>();
            Outer()->HookEventWithCaller<ActorWrapper>(eventName,
                [this](const ActorWrapper& caller, void* params, const std::string& eventName_) {
                    HookPre<ActorWrapper>(caller, params, eventName_);
                });
        }

        Outer()->callbacksPre[eventName].try_emplace(*typeIdx,
            [this, callback](void*, void*, const std::string& eventName_) {
                callback(eventName_);
            });
    }

    template<typename Caller>
    void HookEventWithCaller(const std::string& eventName,
        std::function<void(Caller, void*, std::string)> callback) const
    {
        BM_TRACE_LOG("{:s} hooking {:s} with caller", quote(typeIdx->name()), quote(eventName));

        const auto it = Outer()->callbacksPre.find(eventName);
        if (it == Outer()->callbacksPre.end()) {
            Outer()->callbacksPre[eventName] = std::unordered_map<std::type_index, EventCallback>();
            Outer()->HookEventWithCaller<Caller>(eventName,
                [this](const Caller& caller, void* params, const std::string& eventName_) {
                    HookPre<Caller>(caller, params, eventName_);
                });
        }

        Outer()->callbacksPre[eventName].try_emplace(*typeIdx,
            [this, callback](void* caller, void* params, std::string eventName_) {
                callback(*static_cast<Caller*>(caller), params, eventName_);
            });
    }

    void UnhookEvent(const std::string& eventName) const
    {
        BM_TRACE_LOG("{:s} unhooked {:s}", quote(typeIdx->name()), quote(eventName));

        const auto funcIt = Outer()->callbacksPre.find(eventName);
        if (funcIt == Outer()->callbacksPre.end()) {
            return;
        }
        const auto eventIt = funcIt->second.find(*typeIdx);
        if (eventIt == funcIt->second.end()) {
            return;
        }

        funcIt->second.erase(eventIt);

        if (funcIt->second.empty()) {
            Outer()->UnhookEvent(eventName);
            Outer()->callbacksPre.erase(funcIt);
        }
    }

    template<typename Caller>
    void HookPost(Caller caller, void* params, const std::string& eventName) const
    {
        const auto funcIt = Outer()->callbacksPost.find(eventName);
        if (funcIt != Outer()->callbacksPost.end()) {
            for (auto& [type, func] : funcIt->second) {
                func(static_cast<void*>(&caller), params, eventName);
            }
        }
    }

    void HookEventPost(const std::string& eventName, const std::function<void(std::string)>& callback) const
    {
        BM_TRACE_LOG("{:s} hooking post {:s}", quote(typeIdx->name()), quote(eventName));

        const auto it = Outer()->callbacksPost.find(eventName);
        if (it == Outer()->callbacksPost.end()) {
            Outer()->callbacksPost[eventName] = std::unordered_map<std::type_index, EventCallback>();
            Outer()->HookEventWithCallerPost<ActorWrapper>(eventName,
                [this](const ActorWrapper& caller, void* params, const std::string& eventName_) {
                    HookPost<ActorWrapper>(caller, params, eventName_);
                });
        }

        Outer()->callbacksPost[eventName].try_emplace(*typeIdx,
            [this, callback](void*, void*, const std::string& eventName_) {
                callback(eventName_);
            });
    }

    template<typename Caller>
    void HookEventWithCallerPost(const std::string& eventName, std::function<void(Caller, void*, std::string)> callback) const
    {
        BM_TRACE_LOG("{:s} hooking post {:s} with caller", quote(typeIdx->name()), quote(eventName));

        const auto it = Outer()->callbacksPost.find(eventName);
        if (it == Outer()->callbacksPost.end()) {
            Outer()->callbacksPost[eventName] = std::unordered_map<std::type_index, EventCallback>();
            Outer()->HookEventWithCallerPost<Caller>(eventName,
                [this](const Caller& caller, void* params, const std::string& eventName_) {
                    HookPost<Caller>(caller, params, eventName_);
                });
        }

        Outer()->callbacksPost[eventName].try_emplace(*typeIdx,
            [this, callback](void* caller, void* params, std::string eventName_) {
                callback(*static_cast<Caller*>(caller), params, eventName_);
            });
    }

    void UnhookEventPost(const std::string& eventName) const
    {
        BM_TRACE_LOG("{:s} unhooked post {:s}", quote(typeIdx->name()), quote(eventName));

        const auto funcIt = Outer()->callbacksPost.find(eventName);
        if (funcIt == Outer()->callbacksPost.end()) {
            return;
        }
        const auto eventIt = funcIt->second.find(*typeIdx);
        if (eventIt == funcIt->second.end()) {
            return;
        }

        funcIt->second.erase(eventIt);

        if (funcIt->second.empty()) {
            Outer()->UnhookEventPost(eventName);
            Outer()->callbacksPost.erase(funcIt);
        }
    }

    void SetTimeout(const std::function<void(GameWrapper*)>& theLambda, const float time) const
    {
        Outer()->SetTimeout(theLambda, time);
    }

    void Execute(const std::function<void(GameWrapper*)>& theLambda) const
    {
        Outer()->Execute(theLambda);
    }

    void RegisterNotifier(const std::string& cvar, const std::function<void(std::vector<std::string>)>& notifier,
        const std::string& description, const unsigned char permissions) const
    {
        Outer()->RegisterNotifier(cvar, notifier, description, permissions);
    }

    /* Virtual game mode functions */
    virtual void RenderOptions() {}
    virtual bool IsActive() = 0;
    virtual void Activate(bool) = 0;
    virtual std::string GetGameModeName()
    {
        return "Rocket Plugin Game Mode";
    }

protected:
    bool isActive = false;
    std::unique_ptr<std::type_index> typeIdx;
};
