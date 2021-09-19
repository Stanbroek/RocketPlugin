#pragma once
#include "Version.h"
#include "Networking/Networking.h"

#include "Modules/RocketPluginModule.h"
#include "Modules/GameControls.h"
#include "Modules/LocalMatchSettings.h"
#include "Modules/BotSettings.h"
#include "Modules/BallMods.h"
#include "Modules/PlayerMods.h"
#include "Modules/CarPhysicsMods.h"

constexpr const char* PLUGIN_VERSION = VER_FILE_VERSION_STR;

constexpr char MARKED_INCOMPLETE = '*';
constexpr const char* MARKED_INCOMPLETE_STR = "*";
constexpr const char* NAT_PUNCH_ADDR = "3.3.3.3";
constexpr const char* DEFAULT_GUI_KEYBIND = "Home";
constexpr const char* DEFAULT_CONSTANTS_CONFIG_URL = "https://stanbroek.github.io/RocketPlugin-config/";
constexpr unsigned short DEFAULT_PORT = 7777;

extern std::filesystem::path BakkesModConfigFolder;
extern std::filesystem::path RocketPluginDataFolder;
extern std::filesystem::path RocketLeagueExecutableFolder;

#define BINDS_FILE_PATH        (BakkesModConfigFolder / "binds.cfg")
#define CONFIG_FILE_PATH       (BakkesModConfigFolder / "config.cfg")
#define PRESETS_PATH           (RocketPluginDataFolder / "presets")
#define PRO_TIPS_FILE_PATH     (RocketPluginDataFolder / "Pro-tips.txt")
#define COOKED_PC_CONSOLE_PATH (RocketLeagueExecutableFolder / "../../TAGame/CookedPCConsole")
#define CUSTOM_MAPS_PATH       (COOKED_PC_CONSOLE_PATH / "mods")
#define COPIED_MAPS_PATH       (COOKED_PC_CONSOLE_PATH / "rocketplugin")
#define WORKSHOP_MAPS_PATH     (RocketLeagueExecutableFolder / "../../../../workshop/content/252950")


class RPConfig;
class RocketGameMode;

class RocketPlugin final : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow
{
    friend RPConfig;
    friend RocketGameMode;

    /* Map File Helpers */
public:
private:
    std::vector<std::filesystem::path> getWorkshopMaps(const std::filesystem::path& workshopPath,
        const std::vector<std::string>& extensions = { ".upk", ".udk" },
        const std::string& preferredExtension = ".udk");

    std::shared_ptr<std::string> presetDirPath;
    std::shared_ptr<std::string> workshopMapDirPath;
    std::shared_ptr<std::string> customMapDirPath;

    /* Commandline Parser Helpers */
public:
private:
    void parseArguments(const std::vector<std::string>& arguments);
    void parseJoinArguments(const std::vector<std::string>& arguments);
    void parseHostArguments(const std::vector<std::string>& arguments);
    void parseGameModeArguments(const std::vector<std::string>& arguments);
    void parseMapArguments(const std::vector<std::string>& arguments);
    void parsePlayerCountArguments(const std::vector<std::string>& arguments);
    void parseTeamArguments(const std::vector<std::string>& arguments);
    void parseMutatorArguments(const std::vector<std::string>& arguments);
    void parseRumbleArguments(const std::vector<std::string>& arguments);
    std::vector<std::string> complete(const std::vector<std::string>& arguments);

    /* Steamworks Helpers */
public:
private:
    std::wstring getPlayerNickname(uint64_t uniqueId) const;
    void getSubscribedWorkshopMapsAsync(bool getSubscribedMaps = false);

    struct WorkshopMap
    {
        std::string Title;
        uint64_t Owner = 0;
    };

    std::vector<uint64_t> publishedFileID;
    // Maps workshop id to workshop info.
    std::unordered_map<uint64_t, WorkshopMap> subscribedWorkshopMaps;

    /* Constants Config */
public:
    std::unique_ptr<RPConfig> ConstantsConfig;
private:

    /* Host/Join Match */
public:
    void HostGame(std::string arena = "");
    void JoinGame(const char* pswd = "");

private:
    std::string getGameTags() const;
    void savePreset(const std::string& presetName);
    void loadPreset(const std::filesystem::path& presetPath);
    void resetMutators();
    void setMatchSettings(const std::string& arena = "") const;
    void setMatchMapName(const std::string& arena) const;
    bool setSearchStatus(const std::wstring& searchStatus, bool shouldBroadcast = false) const;
    void broadcastJoining();

    bool isHostingLocalGame() const;
    bool isCurrentMapModded() const;
    bool isMapJoinable(const std::filesystem::path& map);
    bool preLoadMap(const std::filesystem::path& pathName, bool overrideDupe = false, bool warnIfExists = false);
    void copyMap(const std::filesystem::path& map = "");
    void onGameEventInit(const ServerWrapper& server);

    bool isJoiningParty = false;
    std::string joiningPartyIp;
    std::string joiningPartyPswd;
    bool failedToGetMapPackageFileCache = false;

    /* Modules */
public:
    ServerWrapper GetGame(bool allowOnlineGame = false) const;
    bool IsInGame(bool allowOnlineGame = false) const;

    GameControls gameControls;
    LocalMatchSettings matchSettings;
    BotSettings botSettings;
    BallMods ballMods;
    PlayerMods playerMods;
    CarPhysicsMods carPhysicsMods;
private:

    /* BakkesMod Plugin Overrides */
public:
    CATCH_DEFAULT_BM_FUNCTIONS;

    void UnhookEvent(const std::string& eventName) const
    {
        gameWrapper->UnhookEvent(eventName);
    }

    void UnhookEventPost(const std::string& eventName) const
    {
        gameWrapper->UnhookEventPost(eventName);
    }

private:
    void registerCVars();
    void registerNotifiers();
    void registerHooks();
    void registerExternalCVars();
    void registerExternalNotifiers();
    void registerExternalHooks();

    /*
     * Rocket Game Mode functions, implementation is in RocketGameMode.h.
     */
    /* Rocket Game Mode Hooks */
public:
protected:
    using EventCallback = std::function<void(void* caller, void* params, std::string eventName)>;
    std::unordered_map<std::string, std::unordered_map<std::type_index, EventCallback>> callbacksPre;
    std::unordered_map<std::string, std::unordered_map<std::type_index, EventCallback>> callbacksPost;
private:

    /*
     * Render functions, implementation is in PluginManagerGUI.cpp.
     */
    /* Plugin Window Overrides */
public:
    CATCH_RENDER;

    std::string GetMenuName() override;
    std::string GetMenuTitle() override;
    void SetImGuiContext(uintptr_t ctx) override;
    bool ShouldBlockInput() override;
    bool IsActiveOverlay() override;
    void OnOpen() override;
    void OnClose() override;

private:
    bool isWindowOpen = false;
    bool isMinimized = false;
#ifdef DEBUG
    const std::string menuTitle = "Rocket Plugin " + std::string(std::string_view(PLUGIN_VERSION).substr(0, std::string_view(PLUGIN_VERSION).rfind('.'))) + " dev";
#else
    const std::string menuTitle = "Rocket Plugin " + std::string(std::string_view(PLUGIN_VERSION).substr(0, std::string_view(PLUGIN_VERSION).rfind('.')));
#endif

    std::shared_ptr<bool> showDemoWindow;
    std::shared_ptr<bool> showMetricsWindow;

    /* General Settings */
public:
    void PushError(const std::string& message);

private:
    void refreshGameSettingsConstants();
    bool renderCustomMapsSelection(std::map<std::filesystem::path, std::string>& customMaps,
        std::filesystem::path& currentCustomMap, bool& refreshCustomMaps, bool includeWorkshopMaps = true,
        bool includeCustomMaps = true);

    void renderMultiplayerTab();

    std::queue<std::string> errors;
    bool shouldRefreshGameSettingsConstants = true;
    std::future<std::pair<bool, std::string>> gameSettingsRequest;

    /* Host Settings */
public:
    struct GameSetting
    {
        std::string DisplayCategoryName;
        std::string InternalCategoryName;
        size_t CurrentSelected = 0;
        std::vector<std::string> DisplayName;
        std::vector<std::string> InternalName;

        void FixDisplayNames(const GameSetting& other);
        std::string GetSelected() const;
    };

private:
    void renderMultiplayerTabHost();
    void renderMultiplayerTabHostTeamSettings();
    void renderMultiplayerTabHostMutatorSettings();
    void renderMultiplayerTabHostAdvancedSettings();
    void renderMultiplayerTabHostAdvancedSettingsUPnPSettings();
    void renderMultiplayerTabHostAdvancedSettingsP2PSettings();

    void loadRLConstants();

    bool hostingGame = false;
    bool shouldInviteParty = false;
    bool isJoiningHost = false;
    bool enableWorkshopMaps = false;
    bool enableCustomMaps = false;
    GameSetting gameModes;
    bool refreshCustomMapPaths = true;
    // maps / customMapPaths key or map path.
    std::string currentMap;
    // Maps internal name to display name.
    std::map<std::string, std::string> maps;
    bool currentMapIsModded = false;
    // Maps path to display name.
    std::map<std::filesystem::path, std::string> customMapPaths;
    int playerCount = 6;
    GameSetting botDifficulties;
    std::vector<std::filesystem::path> presetPaths;
    std::string hostPswd;
    unsigned short hostPortInternal = DEFAULT_PORT;
    unsigned short hostPortExternal = DEFAULT_PORT;

    /* Join Settings */
public:
private:
    void renderMultiplayerTabJoin();

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

    /* Team Settings */
public:
private:
    int clubColorHues = 0;
    std::vector<ImVec4> clubColors;
    int customColorHues = 0;
    std::vector<ImVec4> customColors;
    ImVec4 defaultBluePrimaryColor;
    ImVec4 defaultBlueAccentColor;
    ImVec4 defaultOrangePrimaryColor;
    ImVec4 defaultOrangeAccentColor;
    std::string team1Name;
    int8_t team1PrimCol = -1;
    int8_t team1AccCol = -1;
    std::string team2Name;
    int8_t team2PrimCol = -1;
    int8_t team2AccCol = -1;
    bool clubMatch = false;

    /* Mutator Settings */
public:
private:
    std::vector<GameSetting> mutators;

    /* Advanced Settings */
public:
private:
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

    /* In Game Mods */
public:
private:
    void renderInGameModsTab();
    void renderInGameModsTabGameEventMods();
    void renderInGameModsTabBallMods();
    void renderInGameModsTabPlayerMods();
    void renderInGameModsTabCarPhysicsMods();

    /* Game Modes */
public:
    template<class Ty>
    std::shared_ptr<Ty> GetCustomGameMode()
    {
        for (const std::shared_ptr<RocketGameMode>& customGameMode : customGameModes) {
            if (Ty* pCustomGameMode = dynamic_cast<Ty*>(customGameMode.get())) {
                return std::shared_ptr<Ty>(customGameMode, pCustomGameMode);
            }
        }

        return std::shared_ptr<Ty>();
    }

private:
    void renderGameModesTab();

    size_t customGameModeSelected = 0;
    std::vector<std::shared_ptr<RocketGameMode>> customGameModes;
};
