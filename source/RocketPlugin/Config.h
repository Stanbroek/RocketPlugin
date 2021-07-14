#pragma once
#include "RocketPlugin.h"


class Config
{
public:
    Config() = default;
    ~Config();
    Config(Config&& other) = delete;
    Config(const Config& other) = delete;
    Config& operator=(Config&& other) = delete;
    Config& operator=(const Config& other) = delete;

    std::shared_future<std::pair<bool, std::string>> Request(const std::string& url,
                                                             std::chrono::seconds timeout = std::chrono::seconds(4));

private:
    std::unordered_map<std::string, std::shared_future<std::pair<bool, std::string>>> activeRequests;
};


class RPConfig final : Config
{
public:
    explicit RPConfig(const std::string& configUrl) : configUrl(configUrl) {}

    static bool ParseGameSettings(const std::string& data, class RocketPlugin* rocketPlugin);
    static bool ParseRumbleItems(const std::string& data, class CrazyRumble* crazyRumble);
    std::future<std::pair<bool, std::string>> RequestGameSettingConstants();
    std::future<std::pair<bool, std::string>> RequestRumbleConstants();

private:
    static void parseGameModes(simdjson::ondemand::document& doc, RocketPlugin::GameSetting& gameModes);
    static void parseBotDifficulties(simdjson::ondemand::document& doc, RocketPlugin::GameSetting& botDifficulties);
    static void parseAvailableMaps(simdjson::ondemand::document& doc, std::map<std::string, std::string>& maps);
    static void parseAvailableColors(simdjson::ondemand::document& doc, int& customColorHues,
                                     std::vector<ImVec4>& customColors, int& clubColorHues,
                                     std::vector<ImVec4>& clubColors, ImVec4& bluePrimaryColor, ImVec4& blueAccentColor,
                                     ImVec4& orangePrimaryColor, ImVec4& orangeAccentColor);
    static void parseAvailableMutators(simdjson::ondemand::document& doc,
                                       std::vector<RocketPlugin::GameSetting>& mutators);
    std::future<bool> requestConfig();

    std::string configUrl;
    std::string gameSettingConstantsConfigUrl;
    std::string rumbleConstantsConfigUrl;
};
