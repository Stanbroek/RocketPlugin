#pragma once
#include "RocketPlugin.h"


class RocketGameMode
{
public:
    explicit RocketGameMode(RocketPlugin* rp) { rocketPlugin = rp; gameWrapper = rp->gameWrapper; }
    virtual ~RocketGameMode() = default;
    RocketGameMode(const RocketGameMode&) = delete;
    RocketGameMode(RocketGameMode&&) = delete;
    RocketGameMode& operator=(const RocketGameMode&) = delete;
    RocketGameMode& operator=(RocketGameMode&&) = delete;

    /* RocketGameMode event hook functions */
    typedef std::function<void(void*, void*, std::string)> Event;

    template <typename Caller>
    void HookPre(Caller caller, void* params, const std::string& eventName)
    {
        auto funcIt = rocketPlugin->callbacksPre.find(eventName);
        if (funcIt != rocketPlugin->callbacksPre.end()) {
            for (auto& [type, func] : funcIt->second) {
                func(static_cast<void*>(&caller), params, eventName);
            }
        }
    }

    void HookEvent(const std::string& eventName, const std::function<void(std::string)>& callback)
    {
        TRACE_LOG("{} hooking {}", quote(_typeid->name()), quote(eventName));

        const auto it = rocketPlugin->callbacksPre.find(eventName);
        if (it == rocketPlugin->callbacksPre.end()) {
            rocketPlugin->callbacksPre[eventName] = std::unordered_map<std::type_index, Event>();
            rocketPlugin->HookEventWithCaller<ActorWrapper>(
                eventName, [this](const ActorWrapper& caller, void* params, const std::string& _eventName) {
                    HookPre<ActorWrapper>(caller, params, _eventName);
                }
            );
        }

        rocketPlugin->callbacksPre[eventName].try_emplace(
            *_typeid, [this, callback](void*, void*, const std::string& _eventName) {
                callback(_eventName);
            });
    }

    template <typename Caller>
    void HookEventWithCaller(const std::string& eventName,
        std::function<void(Caller, void*, std::string)> callback) const
    {
        TRACE_LOG("{} hooking {} with caller", quote(_typeid->name()), quote(eventName));

        const auto it = rocketPlugin->callbacksPre.find(eventName);
        if (it == rocketPlugin->callbacksPre.end()) {
            rocketPlugin->callbacksPre[eventName] = std::unordered_map<std::type_index, Event>();
            rocketPlugin->HookEventWithCaller<Caller>(
                eventName, [this](const Caller& caller, void* params, const std::string& _eventName) {
                    HookPre<Caller>(caller, params, _eventName);
                }
            );
        }

        rocketPlugin->callbacksPre[eventName].try_emplace(
            *_typeid, [this, callback](void* caller, void* params, std::string _eventName) {
                callback(*static_cast<Caller*>(caller), params, _eventName);
            }
        );
    }

    void UnhookEvent(const std::string& eventName) const
    {
        TRACE_LOG("{} unhooked {}", quote(_typeid->name()), quote(eventName));

        auto funcIt = rocketPlugin->callbacksPre.find(eventName);
        if (funcIt == rocketPlugin->callbacksPre.end()) {
            return;
        }
        const auto eventIt = funcIt->second.find(*_typeid);
        if (eventIt == funcIt->second.end()) {
            return;
        }

        funcIt->second.erase(eventIt);

        if (funcIt->second.empty()) {
            gameWrapper->UnhookEvent(eventName);
            rocketPlugin->callbacksPre.erase(funcIt);
        }
    }

    template <typename Caller>
    void HookPost(Caller caller, void* params, const std::string& eventName)
    {
        auto funcIt = rocketPlugin->callbacksPost.find(eventName);
        if (funcIt != rocketPlugin->callbacksPost.end()) {
            for (auto& [type, func] : funcIt->second) {
                func(static_cast<void*>(&caller), params, eventName);
            }
        }
    }

    void HookEventPost(const std::string& eventName, const std::function<void(std::string)>& callback)
    {
        TRACE_LOG("{} hooking post {}", quote(_typeid->name()), quote(eventName));

        const auto it = rocketPlugin->callbacksPost.find(eventName);
        if (it == rocketPlugin->callbacksPost.end()) {
            rocketPlugin->callbacksPost[eventName] = std::unordered_map<std::type_index, Event>();
            rocketPlugin->HookEventWithCallerPost<ActorWrapper>(
                eventName, [this](const ActorWrapper& caller, void* params, const std::string& _eventName) {
                    HookPost<ActorWrapper>(caller, params, _eventName);
                }
            );
        }

        rocketPlugin->callbacksPost[eventName].try_emplace(
            *_typeid, [this, callback](void*, void*, const std::string& _eventName) {
                callback(_eventName);
            }
        );
    }

    template <typename Caller>
    void HookEventWithCallerPost(const std::string& eventName, std::function<void(Caller, void*, std::string)> callback)
    {
        TRACE_LOG("{} hooking post {} with caller", quote(_typeid->name()), quote(eventName));

        const auto it = rocketPlugin->callbacksPost.find(eventName);
        if (it == rocketPlugin->callbacksPost.end()) {
            rocketPlugin->callbacksPost[eventName] = std::unordered_map<std::type_index, Event>();
            rocketPlugin->HookEventWithCallerPost<Caller>(
                eventName, [this](const Caller& caller, void* params, const std::string& _eventName) {
                    HookPost<Caller>(caller, params, _eventName);
                }
            );
        }

        rocketPlugin->callbacksPost[eventName].try_emplace(
            *_typeid, [this, callback](void* caller, void* params, std::string _eventName) {
                callback(*static_cast<Caller*>(caller), params, _eventName);
            }
        );
    }

    void UnhookEventPost(const std::string& eventName) const
    {
        TRACE_LOG("{} unhooked post {}", quote(_typeid->name()), quote(eventName));

        auto funcIt = rocketPlugin->callbacksPost.find(eventName);
        if (funcIt == rocketPlugin->callbacksPost.end()) {
            return;
        }
        const auto eventIt = funcIt->second.find(*_typeid);
        if (eventIt == funcIt->second.end()) {
            return;
        }

        funcIt->second.erase(eventIt);

        if (funcIt->second.empty()) {
            gameWrapper->UnhookEventPost(eventName);
            rocketPlugin->callbacksPost.erase(funcIt);
        }
    }

    /* Virtual game mode functions */
    virtual void RenderOptions() {}
    virtual bool IsActive() = 0;
    virtual void Activate(bool) = 0;
    virtual std::string GetGameModeName() { return "Rocket Plugin Game Mode"; }

protected:
    bool isActive = false;
    RocketPlugin* rocketPlugin = nullptr;
    std::shared_ptr<std::type_index> _typeid;
    std::shared_ptr<GameWrapper> gameWrapper;
};
