#pragma once
#pragma comment(lib, "pluginsdk.lib")
#pragma warning(push, 0)
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "utils/parser.h"
#include "utils/io.h"
#pragma warning(pop)

#include "bakkesmod_additions/cvarmanagerwrapperdebug.h"
#define cvarManager _cvarManager
extern std::shared_ptr<CVarManagerWrapperDebug> cvarManager;
#include "bakkesmod_additions/parser.h"

#include "imgui/imgui.h"
#include "imgui/imgui_searchablecombo.h"
#include "imgui/imgui_rangeslider.h"
#include "imgui/imgui_additions.h"

#include <unordered_map>
#include <filesystem>
#include <errno.h>
#include <regex>

#include "Networking.h"


class RocketGameMode;

class RocketPlugin : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow
{
    public:
        // General Helpers
        static std::string toLower(std::string str, bool changeInline = false);
    private:
        // Match File Helpers
        std::shared_ptr<std::string> presetDirPath;
        std::shared_ptr<std::string> workshopMapDirPath;
        std::shared_ptr<std::string> customMapDirPath;
        bool hasextension(std::string fileextension, std::vector<std::string> extensions);
        std::vector<std::filesystem::path> iterateDirectory(std::string dirPath, std::vector<std::string> extensions = {}, int depth = 0, int maxDepth = 3);
        std::vector<std::filesystem::path> getFilesFromDir(std::string dirPath, int numextension, ...);
        std::vector<std::filesystem::path> getWorkshopMaps(std::string workshopPath, std::string preferedExtension = ".udk");
        
        // Join Match Helpers
        enum class DestAddrType
        {
            UNKNOWN_ADDR,
            INTERAL_ADDR,
            HAMACHI_ADDR,
            EXTERNL_ADDR
        };
        DestAddrType getDestAddrType(const char* addr);
        bool isMapJoinable(std::filesystem::path map);

        // Commandline Parser Helpers
        struct Mutator {
            size_t current_selected;
            const std::string name;
            const std::vector<std::string> display;
            const std::vector<std::string> _hidden;
        };
        void printCommandOptions(std::string error = "");
        void printJoinOptions(std::string error = "");
        void printavailableMutators(std::string error = "");
        int  findSanitizedIndexInMutators(const std::string& e);
        void printavailableMutatorValues(Mutator mutator, std::string error = "");
        int  findSanitizedIndexInMutatorValues(Mutator mutator, const std::string& e);
        void printRumbleOptions(std::string error = "");
        void parseArguments(std::vector<std::string> arguments);
        std::vector<std::string> complete(std::vector<std::string> arguments);

        // Steamworks Helpers
        struct WorkshopMap {
            std::string title;
            uint64_t owner = 0;
        };
        std::vector<uint64_t> publishedFileID;
        std::unordered_map<uint64_t, WorkshopMap> subscribedWorkshopMaps;
        std::wstring GetPlayerNickname(uint64_t uniqueId);
        void GetSubscribedWorkshopMapsAsync(bool getSubscribedMaps = false);
    public:
        // Host Match
        bool setSearchStatus(std::string searchStatus, bool shouldBroadcast = false);
        void setMatchSettings(std::string arena);
        void setMatchMapName(std::string arena);
        void hostGame();
        void onHandleButtonClicked(ActorWrapper modal, void* params);
        void onPartyInvite(ActorWrapper party, void* params);
        void gameEventInit(ServerWrapper server);
        void catchErrorMessage(ActorWrapper shell);
        void joinGame(const char* pswd = "");
        void onProtipMessage(ActorWrapper caller);
        void onPreLoadingScreen(ActorWrapper caller, void* params);
        void broadcastJoining();
        void onPostLoadingScreen();
        void addGodBall();
        void copyMap(std::string map = "");
        bool failedToGetMapPackageFileCache = false;
        void preLoadMap(std::wstring pathName, bool overrideDupe = false, bool warnIfExists = false);
        void savePreset(std::string preset);
        void resetMutators();
        void loadPreset(std::string preset);

        // Game Controls
        ServerWrapper getGame(bool allowOnlineGame = false);
        void forceOvertime();
        void pauseServer();
        void resetMatch();
        void resetPlayers();
        void resetBalls();

        // Match Settings
        void setMaxPlayers(int newNumPlayers);
        int getMaxPlayers();
        void setMaxTeamSize(int newTeamSize);
        int getMaxTeamSize();
        void setRespawnTime(int newRespawnTime);
        int getRespawnTime();
        void setScoreBlue(int newScore);
        int getScoreBlue();
        void setScoreOrange(int newScore);
        int getScoreOrange();
        void setGameTimeRemaining(int newGameTimeRemaining);
        int getGameTimeRemaining();
        void setIsGoalDelayDisabled(bool isGoalDelayDisabled);
        bool getIsGoalDelayDisabled();
        void setIsUnlimitedTime(bool isUnlimitedTime);
        bool getIsUnlimitedTime();

        // Bots
        void setNumBots(bool newNumBots);
        int getNumBots();
        void prepareBots(int newNumBots);
        void setIsAutofilledWithBots(bool isAutofilledWithBots);
        bool getIsAutofilledWithBots();
        void setIsUnfairTeams(bool isUnfairTeams);
        bool getIsUnfairTeams();
        void freezeBots();

        // Ball Mods
        void setNumBalls(int newNumBalls);
        int getNumBalls();
        void setBallsScale(float newBallsScale);
        float getBallsScale();

        // Player Mods
        std::vector<PriWrapper> getPlayers(bool includeBots = false, bool alive = false);
        void setIsAdmin(PriWrapper player, bool isAdmin);
        bool getIsAdmin(PriWrapper player);
        void setIsHidden(PriWrapper player, bool isHidden);
        bool getIsHidden(PriWrapper player);
        void demolish(PriWrapper player);

        // Car Physics mods
        struct CarPhysics {
            float carScale;
            bool carHasCollision;
            bool carIsFrozen;
            float torqueRate;
            float maxCarVelocity;
            float groundStickyForce;
            float wallStickyForce;
        };
        std::vector<std::string> getPlayersNames(std::vector<PriWrapper> players);
        PriWrapper getPlayerFromPlayers(std::vector<PriWrapper> players, size_t currentPlayer);
        void setPhysics(CarWrapper car);
        CarPhysics getPhysics(PriWrapper player);
        void setbCarCollision(PriWrapper player, bool carCollision);
        bool getbCarCollision(PriWrapper player);
        void setCarScale(PriWrapper player, float newCarScale);
        float getCarScale(PriWrapper player);
        void setCarIsFrozen(PriWrapper player, bool carIsFrozen);
        bool getCarIsFrozen(PriWrapper player);
        void setTorqueRate(PriWrapper player, float torqueRate);
        float getTorqueRate(PriWrapper player);
        void setMaxCarVelocity(PriWrapper player, float maxCarVelocity);
        float getMaxCarVelocity(PriWrapper player);
        StickyForceData getStickyForce(PriWrapper player);
        void setGroundStickyForce(PriWrapper player, float groundStickyForce);
        float getGroundStickyForce(PriWrapper player);
        void setWallStickyForce(PriWrapper player, float wallStickyForce);
        float getWallStickyForce(PriWrapper player);

        // RocketGameMode Hooks
        typedef std::function<void(void* caller, void* params, std::string eventName)> Event;
        std::unordered_map<std::string, std::unordered_map<std::type_index, Event>> _callbacksPre;
        std::unordered_map<std::string, std::unordered_map<std::type_index, Event>> _callbacksPost;

        // Keybind function
        void changeKeybind();

        void LoadDebugNotifiers();

        // Bakkesmod functions
        virtual void onLoad();
        virtual void onUnload();

    /*
     * GUI stuff, implementation is in PluginManagerGUI.cpp.
     */
    private:
        // Join settings
        bool isJoiningParty = false;
        std::string joiningPartyIp;
        int joiningPartyPort;
        std::string joiningPartyPswd;
        std::shared_ptr<std::string> ip;
        std::shared_ptr<int> port;
        std::shared_ptr<std::string> password;
        bool joinCustomMap = false;
        int currentJoinMap = 0;
        std::vector<std::filesystem::path> joinableMaps;
        bool loadingScreenHooked = false;
        std::wstring loadingScreenMapName;
        std::wstring loadingScreenMapAuthor;

        // Host settings
        bool hostingGame = false;
        bool shouldInviteParty = false;
        bool isJoiningHost = false;
        bool enableFreeplay = false;
        bool enableWorkshopMaps = false;
        bool enableCustomMaps = false;
        size_t currentGameMode = 0;
        const std::vector<std::string> gameModes = { "Soccar", "Hoops", "Snow Day", "Rumble", "Heatseaker BETA", "Dropshot" };
        const std::vector<std::string> _gameModes = { "Soccar_TA", "Basketball_TA", "Hockey_TA", "Items_TA", "GodBall_TA", "Breakout_TA" };
        int currentMap = 0;
        std::string currentMapFile;
        std::vector<std::filesystem::path> otherMapPaths;
        /* Arena names and their corresponding file names from:
         * https://discordapp.com/channels/327430448596779019/448093289137307658/567129086158438400 
         * and further updated by me. */
        const std::vector<std::string> maps = {
            "DFH Stadium", "Mannfield (Day)", "Champions Field", "Urban Central (Day)",
            "Beckwith Park (Day)", "Utopia Coliseum (Day)", "Wasteland (Day)", "Neo Tokyo",
            "AquaDome", "Starbase ARC", "Salty Shores", "Farmstead", "Forbidden Temple", "DFH Stadium (Stormy)",
            "DFH Stadium (Day)", "Mannfield (Stormy)", "Mannfield (Night)", "Champions Field (day)",
            "Beckwith Park (Stormy)", "Beckwith Park (Midnight)", "Urban Central (Night)",
            "Urban Central (Dawn)", "Utopia Coliseum (Dusk)", "Wasteland (Night)",
            "DFH Stadium (Snowy)", "Mannfield (Snowy)", "Utopia Coliseum (Snowy)",
            "Badlands", "Badlands (Night)", "Salty Shores (Night)", "Tokyo Underpass", "Arctagon",
            "Rivals Arena", "Farmstead (Night)", "Farmstead (The Upside Down)", "Pillars", "Cosmic (Old)", "Cosmic (New)",
            "Double Goal (Old)", "Double Goal (New)", "Octagon (Old)", "Octagon (New)", "Underpass (Old)",
            "Underpass (New)", "Utopia Retro", "Throwback Stadium", "Core 707", "Dunk House", "Neon Fields"
        };
        const std::vector<std::string> _maps = {
            "Stadium_P", "EuroStadium_P", "CS_P", "TrainStation_P", "Park_P",
            "UtopiaStadium_P", "Wasteland_S_P", "NeoTokyo_Standard_P", "Underwater_P",
            "ARC_Standard_P", "Beach_P", "Farm_P", "CHN_Stadium_P", "Stadium_Foggy_P", "Stadium_Day_P",
            "EuroStadium_Rainy_P", "EuroStadium_Night_P", "CS_Day_P", "Park_Rainy_P",
            "Park_Night_P", "TrainStation_Night_P", "TrainStation_Dawn_P",
            "UtopiaStadium_Dusk_P", "Wasteland_Night_S_P", "Stadium_Winter_P",
            "EuroStadium_SnowNight_P", "UtopiaStadium_Snow_P", "Wasteland_P", "Wasteland_Night_P",
            "Beach_Night_P", "NeoTokyo_P", "ARC_P", "CS_HW_P", "Farm_Night_P", "Farm_UpsideDown_P",
            "Labs_CirclePillars_P", "Labs_Cosmic_P", "Labs_Cosmic_V4_P", "Labs_DoubleGoal_P",
            "Labs_DoubleGoal_V2_P","Labs_Octagon_P", "Labs_Octagon_02_P", "Labs_Underpass_v0_p",
            "Labs_Underpass_P", "Labs_Utopia_P", "ThrowbackStadium_P", "ShatterShot_P", "HoopsStadium_P",
            "Music_P"
        };
        int playerCount = 6;
        bool enableBots = false;
        size_t botDifficulty = 0;
        const std::vector<std::string> botDifficulties = { "Rookie", "Pro", "All-Star" };
        const std::vector<std::string> _botDifficulties = { "BotsEasy", "BotsMedium", "BotsHard" };

        // Team settings

        #define IM_VEC3(R, G, B) ImVec4(R, G, B, 1)
        const std::vector<ImVec4> teamColors {
            IM_VEC3(0.900000f, 0.900000f, 0.900000f), IM_VEC3(1.000000f, 0.500000f, 0.500000f), IM_VEC3(1.000000f, 0.625000f, 0.500000f), IM_VEC3(1.000000f, 0.812500f, 0.500000f), IM_VEC3(0.937500f, 1.000000f, 0.500000f), IM_VEC3(0.687500f, 1.000000f, 0.500000f), IM_VEC3(0.500000f, 1.000000f, 0.500000f), IM_VEC3(0.500000f, 1.000000f, 0.700000f), IM_VEC3(0.500000f, 0.916667f, 1.000000f), IM_VEC3(0.500000f, 0.691666f, 1.000000f), IM_VEC3(0.500000f, 0.533333f, 1.000000f), IM_VEC3(0.683334f, 0.500000f, 1.000000f), IM_VEC3(0.900000f, 0.500000f, 1.000000f), IM_VEC3(1.000000f, 0.500000f, 0.816666f), IM_VEC3(1.000000f, 0.500000f, 0.583333f),
            IM_VEC3(0.750000f, 0.750000f, 0.750000f), IM_VEC3(1.000000f, 0.350000f, 0.350000f), IM_VEC3(1.000000f, 0.512500f, 0.350000f), IM_VEC3(1.000000f, 0.756250f, 0.350000f), IM_VEC3(0.918750f, 1.000000f, 0.350000f), IM_VEC3(0.593750f, 1.000000f, 0.350000f), IM_VEC3(0.350000f, 1.000000f, 0.350000f), IM_VEC3(0.350000f, 1.000000f, 0.610000f), IM_VEC3(0.350000f, 0.891666f, 1.000000f), IM_VEC3(0.350000f, 0.599166f, 1.000000f), IM_VEC3(0.350000f, 0.393333f, 1.000000f), IM_VEC3(0.588334f, 0.350000f, 1.000000f), IM_VEC3(0.870000f, 0.350000f, 1.000000f), IM_VEC3(1.000000f, 0.350000f, 0.761666f), IM_VEC3(1.000000f, 0.350000f, 0.458334f),
            IM_VEC3(0.600000f, 0.600000f, 0.600000f), IM_VEC3(1.000000f, 0.200000f, 0.200000f), IM_VEC3(1.000000f, 0.400000f, 0.200000f), IM_VEC3(1.000000f, 0.700000f, 0.200000f), IM_VEC3(0.900000f, 1.000000f, 0.200000f), IM_VEC3(0.500000f, 1.000000f, 0.200000f), IM_VEC3(0.200000f, 1.000000f, 0.200000f), IM_VEC3(0.200000f, 1.000000f, 0.520000f), IM_VEC3(0.200000f, 0.866666f, 1.000000f), IM_VEC3(0.200000f, 0.506666f, 1.000000f), IM_VEC3(0.200000f, 0.253333f, 1.000000f), IM_VEC3(0.493334f, 0.200000f, 1.000000f), IM_VEC3(0.840000f, 0.200000f, 1.000000f), IM_VEC3(1.000000f, 0.200000f, 0.706666f), IM_VEC3(1.000000f, 0.200000f, 0.333334f),
            IM_VEC3(0.400000f, 0.400000f, 0.400000f), IM_VEC3(1.000000f, 0.000000f, 0.000000f), IM_VEC3(1.000000f, 0.250000f, 0.000000f), IM_VEC3(1.000000f, 0.625000f, 0.000000f), IM_VEC3(0.875000f, 1.000000f, 0.000000f), IM_VEC3(0.375000f, 1.000000f, 0.000000f), IM_VEC3(0.000000f, 1.000000f, 0.000000f), IM_VEC3(0.000000f, 1.000000f, 0.400000f), IM_VEC3(0.000000f, 0.833333f, 1.000000f), IM_VEC3(0.000000f, 0.383333f, 1.000000f), IM_VEC3(0.000000f, 0.066667f, 1.000000f), IM_VEC3(0.366667f, 0.000000f, 1.000000f), IM_VEC3(0.800000f, 0.000000f, 1.000000f), IM_VEC3(1.000000f, 0.000000f, 0.633333f), IM_VEC3(1.000000f, 0.000000f, 0.166667f),
            IM_VEC3(0.250000f, 0.250000f, 0.250000f), IM_VEC3(0.700000f, 0.000000f, 0.000000f), IM_VEC3(0.700000f, 0.175000f, 0.000000f), IM_VEC3(0.700000f, 0.437500f, 0.000000f), IM_VEC3(0.612500f, 0.700000f, 0.000000f), IM_VEC3(0.262500f, 0.700000f, 0.000000f), IM_VEC3(0.000000f, 0.700000f, 0.000000f), IM_VEC3(0.000000f, 0.700000f, 0.280000f), IM_VEC3(0.000000f, 0.583333f, 0.700000f), IM_VEC3(0.000000f, 0.268333f, 0.700000f), IM_VEC3(0.000000f, 0.046667f, 0.700000f), IM_VEC3(0.256667f, 0.000000f, 0.700000f), IM_VEC3(0.560000f, 0.000000f, 0.700000f), IM_VEC3(0.700000f, 0.000000f, 0.443333f), IM_VEC3(0.700000f, 0.000000f, 0.116667f),
            IM_VEC3(0.150000f, 0.150000f, 0.150000f), IM_VEC3(0.400000f, 0.000000f, 0.000000f), IM_VEC3(0.400000f, 0.100000f, 0.000000f), IM_VEC3(0.400000f, 0.250000f, 0.000000f), IM_VEC3(0.350000f, 0.400000f, 0.000000f), IM_VEC3(0.150000f, 0.400000f, 0.000000f), IM_VEC3(0.000000f, 0.400000f, 0.000000f), IM_VEC3(0.000000f, 0.400000f, 0.160000f), IM_VEC3(0.000000f, 0.333333f, 0.400000f), IM_VEC3(0.000000f, 0.153333f, 0.400000f), IM_VEC3(0.000000f, 0.026667f, 0.400000f), IM_VEC3(0.146667f, 0.000000f, 0.400000f), IM_VEC3(0.320000f, 0.000000f, 0.400000f), IM_VEC3(0.400000f, 0.000000f, 0.253333f), IM_VEC3(0.400000f, 0.000000f, 0.066667f),
            IM_VEC3(0.020000f, 0.020000f, 0.020000f), IM_VEC3(0.200000f, 0.000000f, 0.000000f), IM_VEC3(0.200000f, 0.050000f, 0.000000f), IM_VEC3(0.200000f, 0.125000f, 0.000000f), IM_VEC3(0.175000f, 0.200000f, 0.000000f), IM_VEC3(0.075000f, 0.200000f, 0.000000f), IM_VEC3(0.000000f, 0.200000f, 0.000000f), IM_VEC3(0.000000f, 0.200000f, 0.080000f), IM_VEC3(0.000000f, 0.166667f, 0.200000f), IM_VEC3(0.000000f, 0.076667f, 0.200000f), IM_VEC3(0.000000f, 0.013333f, 0.200000f), IM_VEC3(0.073333f, 0.000000f, 0.200000f), IM_VEC3(0.160000f, 0.000000f, 0.200000f), IM_VEC3(0.200000f, 0.000000f, 0.126667f), IM_VEC3(0.200000f, 0.000000f, 0.033333f)
        };
        char team1Name[64] = "";
        char team1PrimCol = -1;
        char team1AccCol = -1;
        char team2Name[64] = "";
        char team2PrimCol = -1;
        char team2AccCol = -1;
        bool clubMatch = false;

        // Mutator settings
        size_t currentMatchLength = 0;
        const std::vector<std::string> matchLengths = { "5 Minutes", "11 Minutes", "10 Minutes", "20 Minutes", "Unlimited" };
        const std::vector<std::string> _matchLengths = { "", "10Minutes,", "MaxTime11Minutes,", "20Minutes,", "UnlimitedTime," };
        size_t currentMax_goals = 0;
        const std::vector<std::string> maxGoals = { "Unlimited", "1 Goal", "3 Goals", "5 Goals" };
        const std::vector<std::string> _maxGoals = { "", "Max1,", "Max3,", "Max5," };
        size_t currentOvertime = 0;
        const std::vector<std::string> overtimes = { "Unlimited", "+5 Max, First Score", "+5 Max, Random Team" };
        const std::vector<std::string> _overtimes = { "", "Overtime5MinutesFirstScore,", "Overtime5MinutesRandom," };
        size_t currentSeries_length = 0;
        const std::vector<std::string> seriesLengths = { "Unlimited", "3 Games", "5 Games", "7 Games" };
        const std::vector<std::string> _seriesLengths = { "", "3Games,", "5Games,", "7Games," };
        size_t currentGame_speeds = 0;
        const std::vector<std::string> gameSpeeds = { "Default", "Slo-mo", "Time Warp" };
        const std::vector<std::string> _gameSpeeds = { "", "SloMoGameSpeed,", "SloMoDistanceBall," };
        size_t currentBallAmount = 0;
        const std::vector<std::string> ballAmounts = { "Default", "Two", "Four", "Six" };
        const std::vector<std::string> _ballAmounts = { "", "TwoBalls,", "FourBalls,", "SixBalls," };
        size_t currentBallMaxSpeed = 0;
        const std::vector<std::string> ballMaxSpeeds = { "Default", "Slow", "Fast", "Super Fast" };
        const std::vector<std::string> _ballMaxSpeeds = { "", "SlowBall,", "FastBall,", "SuperFastBall," };
        size_t currentBallType = 0;
        const std::vector<std::string> ballTypes = { "Default", "Cube", "Puck", "Basketball", "Hauntedball", "Beachball" };
        const std::vector<std::string> _ballTypes = { "", "Ball_CubeBall,", "Ball_Puck,", "Ball_BasketBall,", "Ball_Haunted,", "Ball_BeachBall," };
        size_t currentBallPhysics = 0;
        const std::vector<std::string> ballPhysics = { "Default", "Light", "Heavy", "Super Light", "10th Anniversary", "Curveball", "Beach Ball Curve" };
        const std::vector<std::string> _ballPhysics = { "", "LightBall,", "HeavyBall,", "SuperLightBall,", "AnniversaryBall,", "MagnusBall,", "MagnusBeachBall," };
        size_t currentBallSizes = 0;
        const std::vector<std::string> ballSizes = { "Default", "Small", "Medium", "Large", "Gigantic" };
        const std::vector<std::string> _ballSizes = { "", "SmallBall,", "MediumBall,", "BigBall,", "GiantBall," };
        size_t currentBallBounciness = 0;
        const std::vector<std::string> ballBouncinesses = { "Default", "Low", "High", "Super High" };
        const std::vector<std::string> _ballBouncinesses = { "", "LowBounciness,", "HighBounciness,", "SuperBounciness," };
        size_t currentBoostAmount = 0;
        const std::vector<std::string> boostAmounts = { "Default", "No Boost", "Unlimited", "Recharge (slow)", "Recharge (fast)" };
        const std::vector<std::string> _boostAmounts = { "", "NoBooster,", "UnlimitedBooster,", "SlowRecharge,", "RapidRecharge," };
        size_t currentRumble = 0;
        const std::vector<std::string> rumbles = { "None", "Default", "Slow", "Civilized", "Destruction Derby", "Spring Loaded", "Spikes Only", "Haunted Ball Beam", "Rugby" };
        const std::vector<std::string> _rumbles = { "", "ItemsMode,", "ItemsModeSlow,", "ItemsModeBallManipulators,", "ItemsModeCarManipulators,", "ItemsModeSprings,", "ItemsModeSpikes,", "ItemsModeHauntedBallBeam,", "ItemsModeRugby," };
        size_t currentBoostStrength = 0;
        const std::vector<std::string> boostStrengths = { "1X", "1.5X", "2X", "10X" };
        const std::vector<std::string> _boostStrengths = { "", "BoostMultiplier1_5x,", "BoostMultiplier2x,", "BoostMultiplier10x," };
        size_t currentGravity = 0;
        const std::vector<std::string> gravities = { "Default", "Low", "High", "Super High", "Reverse" };
        const std::vector<std::string> _gravities = { "", "LowGravity,", "HighGravity,", "SuperGravity,", "ReverseGravity," };
        size_t currentBallGravity = 0;
        const std::vector<std::string> ballGravities = { "Default", "Low", "High", "Super High" };
        const std::vector<std::string> _ballGravities = { "", "LowGravityBall,", "HighGravityBall,", "SuperGravityBall," };
        size_t currentDemolish = 0;
        const std::vector<std::string> demolishes = { "Default", "Disabled", "Friendly Fire", "On Contact", "On Contact (FF)" };
        const std::vector<std::string> _demolishes = { "", "NoDemolish,", "DemolishAll,", "AlwaysDemolishOpposing,", "AlwaysDemolish," };
        size_t currentRespawnTime = 0;
        const std::vector<std::string> respawnTimes = { "3 Seconds", "2 Seconds", "1 Second", "Disable Goal Reset" };
        const std::vector<std::string> _respawnTimes = { "", "TwoSecondsRespawn,", "OneSecondsRespawn,", "DisableGoalDelay," };
        size_t currentAudio = 0;
        const std::vector<std::string> audio = { "Default", "Haunted" };
        const std::vector<std::string> _audio = { "", "HauntedAudio," };
        size_t currentGameRuleEvents = 0;
        const std::vector<std::string> gameEventsRules = { "Default", "Haunted", "Rugby" };
        const std::vector<std::string> _gameEventsRules = { "", "HauntedGameEventRules,", "RugbyGameEventRules," };

        std::vector<Mutator> mutators = {
            { currentMatchLength, "Match Length", matchLengths, _matchLengths },
            { currentMax_goals, "Max Score", maxGoals, _maxGoals },
            { currentOvertime, "Overtime", overtimes, _overtimes },
            { currentSeries_length, "Series Length", seriesLengths, _seriesLengths },
            { currentGame_speeds, "Game Speed", gameSpeeds, _gameSpeeds },
            { currentBallAmount, "Ball Amount", ballAmounts, _ballAmounts },
            { currentBallMaxSpeed, "Ball Max Speed", ballMaxSpeeds, _ballMaxSpeeds },
            { currentBallType, "Ball Type", ballTypes, _ballTypes },
            { currentBallPhysics, "Ball Physics", ballPhysics, _ballPhysics },
            { currentBallSizes, "Ball size", ballSizes, _ballSizes },
            { currentBallBounciness, "Ball Bounciness", ballBouncinesses, _ballBouncinesses },
            { currentBoostAmount, "Boost Amount", boostAmounts, _boostAmounts },
            { currentRumble, "Rumble", rumbles, _rumbles },
            { currentBoostStrength, "Boost Strength", boostStrengths, _boostStrengths },
            { currentGravity, "Gravity", gravities, _gravities },
            { currentGravity, "Ball Gravity", ballGravities, _ballGravities },
            { currentDemolish, "Demolish", demolishes, _demolishes },
            { currentRespawnTime, "Respawn Time", respawnTimes, _respawnTimes },
            { currentAudio, "Audio", audio, _audio },
            { currentGameRuleEvents, "Game Event Rules", gameEventsRules, _gameEventsRules },
        };

        // Advanced Settings
        bool useUPnP = false;
        std::shared_ptr<UPnPClient> upnpClient;
        int portLeaseDuration = 3600;
        bool hostWithParty = false;
        bool useUDPPunching = false;
        bool useP2P = false;
        std::unique_ptr<P2PHost> p2pHost;
        struct P2PIP { char IP[64]; bool invalidIP; };
        std::vector<P2PIP> connections;

        // Bot mods
        int numBots = 0;

        // Car Physics mods
        size_t selectedPlayer = 0;
        std::unordered_map<unsigned long long, CarPhysics> carPhysics;

        // Gamemodes
        int gamemodeSelected = 0;
        std::vector<std::shared_ptr<RocketGameMode>> gamemodes;

        // Window settings
        bool isWindowOpen = false;
        bool isMinimized = false;
        std::string menuTitle = "Rocket Plugin";
    public:
        virtual void Render();
        virtual std::string GetMenuName();
        virtual std::string GetMenuTitle();
        virtual void SetImGuiContext(uintptr_t ctx);
        virtual bool ShouldBlockInput();
        virtual bool IsActiveOverlay();
        virtual void OnOpen();
        virtual void OnClose();
};


class RocketGameMode
{
public:
    bool isActive = false;
    RocketPlugin* rocketPlugin = nullptr;
    std::shared_ptr<std::type_index> _typeid;
    std::shared_ptr<GameWrapper> gameWrapper;

    RocketGameMode(RocketPlugin* rp) { rocketPlugin = rp; gameWrapper = rp->gameWrapper; }
    ~RocketGameMode() { Activate(false); }

    // RocketGameMode event hook functions
    typedef std::function<void(void* caller, void* params, std::string eventName)> Event;

    template<typename Caller>
    void HookPre(Caller caller, void* params, std::string eventName)
    {
        auto funcIt = rocketPlugin->_callbacksPre.find(eventName);
        if (funcIt != rocketPlugin->_callbacksPre.end()) {
            for (auto &eventIt : funcIt->second) {
                eventIt.second((void*)&caller, params, eventName);
            }
        }
    }

    void HookEvent(std::string eventName, std::function<void(std::string eventName)> callback)
    {
        cvarManager->info_log("HookEvent hooking for " + std::string(_typeid->name()));

        auto it = rocketPlugin->_callbacksPre.find(eventName);
        if (it == rocketPlugin->_callbacksPre.end()) {
            rocketPlugin->_callbacksPre[eventName] = std::unordered_map<std::type_index, Event>();
            gameWrapper->HookEventWithCaller<ActorWrapper>(eventName, std::bind(&RocketGameMode::HookPre<ActorWrapper>, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        }

        rocketPlugin->_callbacksPre[eventName].try_emplace(*_typeid, [this, callback](void*, void*, std::string eventName) {
            callback(eventName);
        });
    }

    template<typename Caller>
    void HookEventWithCaller(std::string eventName, std::function<void(Caller caller, void* params, std::string eventName)> callback)
    {
        cvarManager->info_log("HookEventWithCaller hooking for " + std::string(_typeid->name()));

        auto it = rocketPlugin->_callbacksPre.find(eventName);
        if (it == rocketPlugin->_callbacksPre.end()) {
            rocketPlugin->_callbacksPre[eventName] = std::unordered_map<std::type_index, Event>();
            gameWrapper->HookEventWithCaller<Caller>(eventName, std::bind(&RocketGameMode::HookPre<Caller>, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        }

        rocketPlugin->_callbacksPre[eventName].try_emplace(*_typeid, [this, callback](void* caller, void* params, std::string eventName) {
            callback(*(Caller*)caller, params, eventName);
        });
    }

    void UnhookEvent(std::string eventName)
    {
        cvarManager->info_log("UnhookEvent unhooking for " + std::string(_typeid->name()));

        auto funcIt = rocketPlugin->_callbacksPre.find(eventName);
        if (funcIt == rocketPlugin->_callbacksPre.end()) {
            return;
        }
        auto eventIt = funcIt->second.find(*_typeid);
        if (eventIt == funcIt->second.end()) {
            return;
        }

        funcIt->second.erase(eventIt);

        if (funcIt->second.empty()) {
            gameWrapper->UnhookEvent(eventName);
            rocketPlugin->_callbacksPre.erase(funcIt);
        }
    }

    template<typename Caller>
    void HookPost(Caller caller, void* params, std::string eventName)
    {
        auto funcIt = rocketPlugin->_callbacksPost.find(eventName);
        if (funcIt != rocketPlugin->_callbacksPost.end()) {
            for (auto &eventIt : funcIt->second) {
                eventIt.second((void*)&caller, params, eventName);
            }
        }
    }

    void HookEventPost(std::string eventName, std::function<void(std::string eventName)> callback)
    {
        cvarManager->info_log("HookEventPost hooking for " + std::string(_typeid->name()));

        auto it = rocketPlugin->_callbacksPost.find(eventName);
        if (it == rocketPlugin->_callbacksPost.end()) {
            rocketPlugin->_callbacksPost[eventName] = std::unordered_map<std::type_index, Event>();
            gameWrapper->HookEventWithCallerPost<ActorWrapper>(eventName, std::bind(&RocketGameMode::HookPost<ActorWrapper>, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        }

        rocketPlugin->_callbacksPost[eventName].try_emplace(*_typeid, [this, callback](void*, void*, std::string eventName) {
            callback(eventName);
        });
    }

    template<typename Caller>
    void HookEventWithCallerPost(std::string eventName, std::function<void(Caller caller, void* params, std::string eventName)> callback)
    {
        cvarManager->info_log("HookEventWithCallerPost hooking for " + std::string(_typeid->name()));

        auto it = rocketPlugin->_callbacksPost.find(eventName);
        if (it == rocketPlugin->_callbacksPost.end()) {
            rocketPlugin->_callbacksPost[eventName] = std::unordered_map<std::type_index, Event>();
            gameWrapper->HookEventWithCallerPost<Caller>(eventName, std::bind(&RocketGameMode::HookPost<Caller>, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        }

        rocketPlugin->_callbacksPost[eventName].try_emplace(*_typeid, [this, callback](void* caller, void* params, std::string eventName) {
            callback(*(Caller*)caller, params, eventName);
        });
    }

    void UnhookEventPost(std::string eventName)
    {
        cvarManager->info_log("UnhookEventPost unhooking for " + std::string(_typeid->name()));

        auto funcIt = rocketPlugin->_callbacksPost.find(eventName);
        if (funcIt == rocketPlugin->_callbacksPost.end()) {
            return;
        }
        auto eventIt = funcIt->second.find(*_typeid);
        if (eventIt == funcIt->second.end()) {
            return;
        }

        funcIt->second.erase(eventIt);

        if (funcIt->second.empty()) {
            gameWrapper->UnhookEventPost(eventName);
            rocketPlugin->_callbacksPost.erase(funcIt);
        }
    }

    // Virtual gamemode functions
    virtual void RenderOptions() {};
    virtual bool IsActive() { return isActive; };
    virtual void Activate(bool) {};
    virtual std::string GetGamemodeName() { return "Rocket Plugin Gamemode"; };
};