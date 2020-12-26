#pragma once
#pragma comment(lib, "pluginsdk.lib")
#pragma warning(push, 0)
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "utils/parser.h"
#include "utils/io.h"
#pragma warning(pop)

#include "utils/cvarmanagerwrapperdebug.h"
#include "utils/se_exception.h"
#include "utils/stringify.h"
#include "utils/parser_w.h"

#include "imgui/imgui.h"
#include "imgui/imgui_searchablecombo.h"
#include "imgui/imgui_rangeslider.h"
#include "imgui/imgui_additions.h"

#include <unordered_map>
#include <filesystem>
#include <utility>
#include <regex>

#include "Version.h"
#include "Networking.h"

constexpr const char* PLUGIN_VERSION = VER_FILE_VERSION_STR;

constexpr char MARKED_INCOMPLETE = '*';
constexpr const char* MARKED_INCOMPLETE_STR = "*";
constexpr const char* NAT_PUNCH_ADDR = "3.3.3.3";
constexpr const char* DEFAULT_GUI_KEYBIND = "Home";
constexpr unsigned short DEFAULT_PORT = 7777;

extern std::filesystem::path BakkesModConfigFolder;
extern std::filesystem::path RocketPluginDataFolder;
extern std::filesystem::path RocketLeagueExecutableFolder;

#define BINDS_FILE_PATH     (BakkesModConfigFolder / "binds.cfg")
#define CONFIG_FILE_PATH    (BakkesModConfigFolder / "config.cfg")
#define PRESETS_PATH        (RocketPluginDataFolder / "presets")
#define PRO_TIPS_FILE_PATH  (RocketPluginDataFolder / "Pro-tips.txt")
#define CUSTOM_MAPS_PATH    (RocketLeagueExecutableFolder / "../../TAGame/CookedPCConsole/mods")
#define WORKSHOP_MAPS_PATH  (RocketLeagueExecutableFolder / "../../../../workshop/content/252950")


class RocketGameMode;

class RocketPlugin final : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow
{
public:
    static bool HasExtension(const std::string& fileExtension, const std::vector<std::string>& extensions);
    static std::vector<std::filesystem::path> IterateDirectory(const std::filesystem::path& directory,
                                                               const std::vector<std::string>& extensions,
                                                               int depth = 0, int maxDepth = 3);
    static std::vector<std::filesystem::path> GetFilesFromDir(const std::filesystem::path& directory, 
                                                              int numExtension, ...);
private:
    /* Match File Helpers */
    bool isMapJoinable(const std::filesystem::path&) const;
    std::vector<std::filesystem::path> getWorkshopMaps(const std::filesystem::path& workshopPath,
                                                       const std::vector<std::string>& extensions = { ".upk", ".udk" },
                                                       const std::string& preferredExtension = ".udk");

    std::shared_ptr<std::string> presetDirPath;
    std::shared_ptr<std::string> workshopMapDirPath;
    std::shared_ptr<std::string> customMapDirPath;

    /* Commandline Parser Helpers */
    void parseArguments(std::vector<std::string> arguments);
    std::vector<std::string> complete(std::vector<std::string> arguments);

    /* Steamworks Helpers */
    struct WorkshopMap {
        std::string Title;
        uint64_t Owner = 0;
    };
    std::vector<uint64_t> publishedFileID;
    // Maps workshop id to workshop info.
    std::unordered_map<uint64_t, WorkshopMap> subscribedWorkshopMaps;  
    std::wstring getPlayerNickname(uint64_t) const;
    void getSubscribedWorkshopMapsAsync(bool = false) const;

public:
    /* Host/Join Match */
    // TODO, reorder in the cpp files.
    void hostGame();
    void savePreset(const std::string& presetName);
    void loadPreset(const std::filesystem::path& presetPath);
    void resetMutators();
    void setMatchSettings(const std::string&) const;
    void setMatchMapName(const std::string&) const;
    bool setSearchStatus(const std::wstring&, bool = false) const;
    void broadcastJoining() const;

    void joinGame(const char* pswd = "");
    void onPartyInvite(void*) const;
    void onHandleButtonClicked(const ActorWrapper&, void*) const;
    void catchErrorMessage(const ActorWrapper&) const;
    bool clearSearchStatus() const;

    bool preLoadMap(const std::filesystem::path&, bool = false, bool = false) const;
    void copyMap(const std::filesystem::path& map = "");
    void gameEventInit(const ServerWrapper& server);
    void onPreLoadingScreen(const ActorWrapper&, void*) const;
    void onProTipMessage(const ActorWrapper&) const;
    void onPostLoadingScreen();

    bool isJoiningParty = false;
    std::string joiningPartyIp;
    std::string joiningPartyPswd;
    bool FailedToGetMapPackageFileCache = false;

    /* Game Controls */
    ServerWrapper getGame(bool allowOnlineGame = false) const;
    void forceOvertime() const;
    void pauseServer() const;
    void resetMatch() const;
    void resetPlayers() const;
    void resetBalls() const;

    /* Match Settings */
    void setMaxPlayers(int newNumPlayers) const;
    int getMaxPlayers() const;
    void setMaxTeamSize(int newTeamSize) const;
    int getMaxTeamSize() const;
    void setRespawnTime(int newRespawnTime) const;
    int getRespawnTime() const;
    void setScore(int team, int newScore) const;
    int getScore(int team) const;
    void setScoreBlue(int newScore) const;
    int getScoreBlue() const;
    void setScoreOrange(int newScore) const;
    int getScoreOrange() const;
    void setGameTimeRemaining(int newGameTimeRemaining) const;
    int getGameTimeRemaining() const;
    void setIsGoalDelayDisabled(bool isGoalDelayDisabled) const;
    bool getIsGoalDelayDisabled() const;
    void setIsUnlimitedTime(bool isUnlimitedTime) const;
    bool getIsUnlimitedTime() const;

    /* Bots */
    void setMaxNumBots(bool newMaxNumBots) const;
    int getMaxNumBots() const;
    void prepareBots(int newMaxNumBotsPerTeam) const;
    void setIsAutoFilledWithBots(bool isAutoFilledWithBots) const;
    bool getIsAutoFilledWithBots() const;
    void setIsUnfairTeams(bool isUnfairTeams) const;
    bool getIsUnfairTeams() const;
    void freezeBots() const;

    /* Ball Mods */
    void setNumBalls(int newNumBalls) const;
    int getNumBalls() const;
    void setBallsScale(float newBallsScale) const;
    float getBallsScale() const;

    /* Player Mods */
    std::vector<PriWrapper> getPlayers(bool includeBots = false, bool mustBeAlive = false) const;
    std::vector<std::string> getPlayersNames(bool includeBots = false, bool mustBeAlive = false) const;
    std::vector<std::string> getPlayersNames(const std::vector<PriWrapper>& players) const;
    void setIsAdmin(PriWrapper player, bool isAdmin) const;
    bool getIsAdmin(PriWrapper player) const;
    void setIsHidden(PriWrapper player, bool isHidden) const;
    bool getIsHidden(PriWrapper player) const;
    void demolish(PriWrapper player) const;

    /* Car Physics mods */
    struct CarPhysics {
        float CarScale;
        bool  CarHasCollision;
        bool  CarIsFrozen;
        float TorqueRate;
        float MaxCarVelocity;
        float GroundStickyForce;
        float WallStickyForce;
    };
    void setPhysics(CarWrapper car);
    CarPhysics getPhysics(PriWrapper player);
    void setbCarCollision(PriWrapper player, bool carHasCollision);
    bool getbCarCollision(PriWrapper player) const;
    void setCarScale(PriWrapper player, float newCarScale);
    float getCarScale(PriWrapper player) const;
    void setCarIsFrozen(PriWrapper player, bool carIsFrozen);
    bool getCarIsFrozen(PriWrapper player) const;
    void setTorqueRate(PriWrapper player, float torqueRate);
    float getTorqueRate(PriWrapper player) const;
    void setMaxCarVelocity(PriWrapper player, float maxCarVelocity);
    float getMaxCarVelocity(PriWrapper player) const;
    StickyForceData getStickyForce(PriWrapper player) const;
    void setGroundStickyForce(PriWrapper player, float groundStickyForce);
    float getGroundStickyForce(PriWrapper player) const;
    void setWallStickyForce(PriWrapper player, float wallStickyForce);
    float getWallStickyForce(PriWrapper player) const;

    /* RocketGameMode Hooks */
    typedef std::function<void(void* caller, void* params, std::string eventName)> Event;
    std::unordered_map<std::string, std::unordered_map<std::type_index, Event>> _callbacksPre;
    std::unordered_map<std::string, std::unordered_map<std::type_index, Event>> _callbacksPost;

    void loadDebugNotifiers();

    /* BakkesMod functions */
    void onLoad() override;
    void onUnload() override;

    /* Game wrapper functions */
    template<typename T, typename std::enable_if<std::is_base_of<ObjectWrapper, T>::value>::type* = nullptr>
    void HookEventWithCaller(const std::string& eventName,
                             const std::function<void(T, void*, const std::string&)>& callback)
    {
        gameWrapper->HookEventWithCaller<T>(eventName,
            [=](T caller, void* params, const std::string& _eventName) {
                try {
                    callback(caller, params, _eventName);
                }
                catch (const SE_Exception& e) {
                    GlobalCVarManager->critical_log(quote(_eventName) + " hook threw c exception: " + e.getSeMessage());
                }
                catch (const std::exception& e) {
                    GlobalCVarManager->critical_log(quote(_eventName) + " hook threw exception: " + e.what());
                }
                catch (...) {
                    GlobalCVarManager->critical_log(quote(_eventName) + " hook threw an exception");
                }
            });
    }

    template<typename T, typename std::enable_if<std::is_base_of<ObjectWrapper, T>::value>::type* = nullptr>
    void HookEventWithCallerPost(const std::string& eventName,
                                 const std::function<void(T, void*, const std::string&)>& callback)
    {
        gameWrapper->HookEventWithCallerPost<T>(eventName,
            [=](T caller, void* params, const std::string& _eventName) {
                try {
                    callback(caller, params, _eventName);
                }
                catch (const SE_Exception& e) {
                    GlobalCVarManager->critical_log(quote(_eventName) + " post hook threw c exception: " + e.getSeMessage());
                }
                catch (const std::exception& e) {
                    GlobalCVarManager->critical_log(quote(_eventName) + " post hook threw exception: " + e.what());
                }
                catch (...) {
                    GlobalCVarManager->critical_log(quote(_eventName) + " post hook threw an exception");
                }
            });
    }

    void Execute(const std::function<void(GameWrapper*)>& theLambda)
    {
        gameWrapper->Execute([=](GameWrapper* gw) {
            try {
                theLambda(gw);
            }
            catch (const SE_Exception& e) {
                GlobalCVarManager->critical_log(std::string("Execute threw c exception: ") + e.getSeMessage());
            }
            catch (const std::exception& e) {
                GlobalCVarManager->critical_log(std::string("Execute threw exception: ") + e.what());
            }
            catch (...) {
                GlobalCVarManager->critical_log("Execute threw an exception");
            }
        });
    }

    /* CVar manager functions */
    void RegisterNotifier(const std::string& cvar, const std::function<void(std::vector<std::string>)>& notifier,
                          const std::string& description, unsigned char permissions)
    {
        cvarManager->registerNotifier(cvar, [=](const std::vector<std::string>& arguments) {
            try {
                notifier(arguments);
            }
            catch (const SE_Exception& e) {
                GlobalCVarManager->critical_log(quote(cvar) + " notifier threw c exception: " + e.getSeMessage());
            }
            catch (const std::exception& e) {
                GlobalCVarManager->critical_log(quote(cvar) + " notifier threw exception: " + e.what());
            }
            catch (...) {
                GlobalCVarManager->critical_log(quote(cvar) + " notifier threw an exception");
            }
        }, description, permissions);
    }

/*
 * GUI stuff, implementation is in PluginManagerGUI.cpp.
 */
public:
    struct GameSetting {
        std::string DisplayCategoryName;
        std::string InternalCategoryName;
        size_t CurrentSelected = 0;
        std::vector<std::string> DisplayName;
        std::vector<std::string> InternalName;

        void FixDisplayNames(const GameSetting& other);
        std::string GetSelected() const;
    };

private:
    std::shared_ptr<bool> showDemoWindow;
    std::shared_ptr<bool> showMetricsWindow;
    std::queue<std::string> errors;

    /* Join settings */
    int joiningPartyPort = DEFAULT_PORT;
    std::shared_ptr<std::string> joinIP;
    std::shared_ptr<int> joinPort;
    bool joinCustomMap = false;
    bool refreshJoinableMaps = true;
    // joinableMaps key.
    std::filesystem::path currentJoinMap;
    // Maps path to display name.
    std::map<std::filesystem::path, std::string> joinableMaps;
    bool loadingScreenHooked = false;
    std::wstring loadingScreenMapName;
    std::wstring loadingScreenMapAuthor;

    /* Host settings */
    void loadRLConstants();

    bool hostingGame = false;
    bool shouldInviteParty = false;
    bool isJoiningHost = false;
    bool enableWorkshopMaps = false;
    bool enableCustomMaps = false;
    GameSetting gameModes;
    bool refreshRLConstants = true;
    bool refreshCustomMapPaths = true;
    // maps or customMapPaths key.
    std::string currentMap;
    // Maps internal name to display name.
    std::map<std::string, std::string> maps;
    // Maps path to display name.
    std::map<std::filesystem::path, std::string> customMapPaths;
    int playerCount = 6;
    GameSetting botDifficulties;
    std::vector<std::filesystem::path> presetPaths;
    std::string hostPswd;
    unsigned short hostPortInternal = DEFAULT_PORT;
    unsigned short hostPortExternal = DEFAULT_PORT;

    /* Team settings */
    int customColorHues = 0;
    std::vector<ImVec4> customColors;
    int clubColorHues = 0;
    std::vector<ImVec4> clubColors;
    ImVec4 defaultBluePrimaryColor;
    ImVec4 defaultBlueAccentColor;
    ImVec4 defaultOrangePrimaryColor;
    ImVec4 defaultOrangeAccentColor;
    char team1Name[64] = "";
    char team1PrimCol = -1;
    char team1AccCol = -1;
    char team2Name[64] = "";
    char team2PrimCol = -1;
    char team2AccCol = -1;
    bool clubMatch = false;

    /* Mutator settings */
    std::vector<GameSetting> mutators;

    /* Advanced Settings */
    bool useUPnP = false;
    std::shared_ptr<UPnPClient> upnpClient;
    int portLeaseDuration = 60 * 60;
    bool hostWithParty = false;
    bool useUDPPunching = false;
    bool useP2P = false;
    std::shared_ptr<P2PHost> p2pHost;

    struct P2PIP
    {
        char IP[64];
        bool InvalidIP;
    };
    std::vector<P2PIP> connections;

    /* Car Physics mods */
    size_t selectedPlayer = 0;
    std::unordered_map<uint64_t, CarPhysics> carPhysics;

    /* Game modes */
    size_t customGameModeSelected = 0;
    std::vector<std::shared_ptr<RocketGameMode>> customGameModes;

    /* Window settings */
    void Render() override;
    void renderMultiplayerTab();
    void renderMultiplayerTabHost();
    void renderMultiplayerTabJoin();
    void renderInGameModsTab();
    void renderGameModesTab();
    bool renderCustomMapsSelection(std::map<std::filesystem::path, std::string>& customMaps,
                                   std::filesystem::path& currentCustomMap, bool& refreshCustomMaps,
                                   bool includeWorkshopMaps = true, bool includeCustomMaps = true);
    std::string GetMenuName() override;
    std::string GetMenuTitle() override;
    void SetImGuiContext(uintptr_t ctx) override;
    bool ShouldBlockInput() override;
    bool IsActiveOverlay() override;
    void OnOpen() override;
    void OnClose() override;

    bool isWindowOpen = false;
    bool isMinimized = false;
    std::string menuTitle = concat("Rocket Plugin ", std::string(PLUGIN_VERSION).substr(0, std::string(PLUGIN_VERSION).rfind('.')));
};


class RocketGameMode
{
public:
    explicit RocketGameMode(RocketPlugin* rp) { rocketPlugin = rp; gameWrapper = rp->gameWrapper; }
    virtual ~RocketGameMode() = default;
    RocketGameMode(const RocketGameMode&) = delete;
    RocketGameMode(RocketGameMode&&)      = delete;
    RocketGameMode& operator=(const RocketGameMode&) = delete;
    RocketGameMode& operator=(RocketGameMode&&)      = delete;

    /* RocketGameMode event hook functions */
    typedef std::function<void(void*, void*, std::string)> Event;

    template <typename Caller>
    void HookPre(Caller caller, void* params, const std::string& eventName)
    {
        auto funcIt = rocketPlugin->_callbacksPre.find(eventName);
        if (funcIt != rocketPlugin->_callbacksPre.end()) {
            for (auto& [type, func] : funcIt->second) {
                func(static_cast<void*>(&caller), params, eventName);
            }
        }
    }

    void HookEvent(const std::string& eventName, const std::function<void(std::string)>& callback)
    {
        TRACE_LOG("hooking " + quote(_typeid->name()));

        const auto it = rocketPlugin->_callbacksPre.find(eventName);
        if (it == rocketPlugin->_callbacksPre.end()) {
            rocketPlugin->_callbacksPre[eventName] = std::unordered_map<std::type_index, Event>();
            rocketPlugin->HookEventWithCaller<ActorWrapper>(
                eventName, [this](const ActorWrapper& caller, void* params, const std::string& _eventName) {
                    HookPre<ActorWrapper>(caller, params, _eventName);
                });
        }

        rocketPlugin->_callbacksPre[eventName].try_emplace(
            *_typeid, [this, callback](void*, void*, const std::string& _eventName) {
                callback(_eventName);
            });
    }

    template <typename Caller>
    void HookEventWithCaller(const std::string& eventName,
                             std::function<void(Caller, void*, std::string)> callback) const
    {
        TRACE_LOG("hooking " + quote(_typeid->name()));

        const auto it = rocketPlugin->_callbacksPre.find(eventName);
        if (it == rocketPlugin->_callbacksPre.end()) {
            rocketPlugin->_callbacksPre[eventName] = std::unordered_map<std::type_index, Event>();
            rocketPlugin->HookEventWithCaller<Caller>(
                eventName, [this](const Caller& caller, void* params, const std::string& _eventName) {
                    HookPre<Caller>(caller, params, _eventName);
                });
        }

        rocketPlugin->_callbacksPre[eventName].try_emplace(
            *_typeid, [this, callback](void* caller, void* params, std::string _eventName) {
                callback(*static_cast<Caller*>(caller), params, _eventName);
            });
    }

    void UnhookEvent(const std::string& eventName) const
    {
        TRACE_LOG("unhooking " + quote(_typeid->name()));

        auto funcIt = rocketPlugin->_callbacksPre.find(eventName);
        if (funcIt == rocketPlugin->_callbacksPre.end()) {
            return;
        }
        const auto eventIt = funcIt->second.find(*_typeid);
        if (eventIt == funcIt->second.end()) {
            return;
        }

        funcIt->second.erase(eventIt);

        if (funcIt->second.empty()) {
            gameWrapper->UnhookEvent(eventName);
            rocketPlugin->_callbacksPre.erase(funcIt);
        }
    }

    template <typename Caller>
    void HookPost(Caller caller, void* params, const std::string& eventName)
    {
        auto funcIt = rocketPlugin->_callbacksPost.find(eventName);
        if (funcIt != rocketPlugin->_callbacksPost.end()) {
            for (auto& [type, func] : funcIt->second) {
                func(static_cast<void*>(&caller), params, eventName);
            }
        }
    }

    void HookEventPost(const std::string& eventName, const std::function<void(std::string)>& callback)
    {
        TRACE_LOG("hooking " + quote(_typeid->name()));

        const auto it = rocketPlugin->_callbacksPost.find(eventName);
        if (it == rocketPlugin->_callbacksPost.end()) {
            rocketPlugin->_callbacksPost[eventName] = std::unordered_map<std::type_index, Event>();
            rocketPlugin->HookEventWithCallerPost<ActorWrapper>(
                eventName, [this](const ActorWrapper& caller, void* params, const std::string& _eventName) {
                    HookPost<ActorWrapper>(caller, params, _eventName);
                });
        }

        rocketPlugin->_callbacksPost[eventName].try_emplace(
            *_typeid, [this, callback](void*, void*, const std::string& _eventName) {
                callback(_eventName);
            });
    }

    template <typename Caller>
    void HookEventWithCallerPost(const std::string& eventName, std::function<void(Caller, void*, std::string)> callback)
    {
        TRACE_LOG("hooking " + quote(_typeid->name()));

        const auto it = rocketPlugin->_callbacksPost.find(eventName);
        if (it == rocketPlugin->_callbacksPost.end()) {
            rocketPlugin->_callbacksPost[eventName] = std::unordered_map<std::type_index, Event>();
            rocketPlugin->HookEventWithCallerPost<Caller>(
                eventName, [this](const Caller& caller, void* params, const std::string& _eventName) {
                    HookPost<Caller>(caller, params, _eventName);
                });
        }

        rocketPlugin->_callbacksPost[eventName].try_emplace(
            *_typeid, [this, callback](void* caller, void* params, std::string _eventName) {
                callback(*static_cast<Caller*>(caller), params, _eventName);
            });
    }

    void UnhookEventPost(const std::string& eventName) const
    {
        TRACE_LOG("unhooking " + quote(_typeid->name()));

        auto funcIt = rocketPlugin->_callbacksPost.find(eventName);
        if (funcIt == rocketPlugin->_callbacksPost.end()) {
            return;
        }
        const auto eventIt = funcIt->second.find(*_typeid);
        if (eventIt == funcIt->second.end()) {
            return;
        }

        funcIt->second.erase(eventIt);

        if (funcIt->second.empty()) {
            gameWrapper->UnhookEventPost(eventName);
            rocketPlugin->_callbacksPost.erase(funcIt);
        }
    }

    /* Virtual game mode functions */
    virtual void RenderOptions() {}
    virtual bool IsActive() { return isActive; }
    virtual void Activate(bool) {}
    virtual std::string GetGameModeName() { return "Rocket Plugin Game Mode"; }

protected:
    bool isActive = false;
    RocketPlugin* rocketPlugin = nullptr;
    std::shared_ptr<std::type_index> _typeid;
    std::shared_ptr<GameWrapper> gameWrapper;
};
