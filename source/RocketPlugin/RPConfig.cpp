// RPConfig.cpp
// Online config for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.6.8 18/09/21
// BMSDK version: 95
#include "RPConfig.h"

#include "GameModes/CrazyRumble.h"

constexpr int HttpStatusCodeSuccessOk = 200;


/// <summary>Waits until all request are finished.</summary>
BaseConfig::~BaseConfig()
{
    for (const auto& [url, request] : activeRequests) {
        if (request.valid()) {
            request.wait();
        }
    }
}


/// <summary>Sends a http request.</summary>
/// <param name="url">Url to send a http request to</param>
/// <param name="timeout">Timeout for the http request, HttpWrapper has a timeout of 3s</param>
/// <returns>Future with the http request response</returns>
std::shared_future<std::pair<bool, std::string>> BaseConfig::Request(const std::string& url,
    std::chrono::seconds timeout)
{
    BM_TRACE_LOG(quote(url));
    if (const auto& it = activeRequests.find(url); it != activeRequests.end()) {
        return it->second;
    }

    std::shared_future<std::pair<bool, std::string>> future = save_promise<std::pair<bool, std::string>>("",
        [this, url, timeout]() {
            std::mutex                   mutex;
            std::condition_variable      condVar;
            std::pair<bool, std::string> response;

            CurlRequest request;
            request.url = url;
            HttpWrapper::SendCurlJsonRequest(
                request, [this, &response, &condVar](const int httpStatusCode, const std::string& data) {
                    BM_TRACE_LOG("{:d}", httpStatusCode);
                    response = std::pair(httpStatusCode == HttpStatusCodeSuccessOk, data);
                    condVar.notify_all();
                });

            // Because we do not own the object that holds our callback, so we have to wait until it is finished.
            // But if the request errors, we will never hear about it, so we also set a timeout and cross our fingers.
            std::unique_lock<std::mutex> lock(mutex);
            const std::cv_status status = condVar.wait_for(lock, timeout);
            if (status == std::cv_status::timeout) {
                BM_ERROR_LOG("request to {:s} timed out", quote(url));
                return std::pair(false, std::string());
            }

            return response;
        }).share();

    activeRequests[url] = future;

    return future;
}


/// <summary>Parses game settings json string into Rocket Plugin variables.</summary>
/// <param name="data">Json string with game settings</param>
/// <param name="rocketPlugin">Rocket Plugin class to store the result in</param>
/// <returns>Bool with if the game settings where parsed successfully</returns>
bool RPConfig::ParseGameSettings(const std::string& data, RocketPlugin* rocketPlugin)
{
    simdjson::ondemand::parser    parser;
    const simdjson::padded_string paddedData = data;
    simdjson::ondemand::document  doc        = parser.iterate(paddedData);

    try {
        parseGameModes(doc, rocketPlugin->gameModes);
        parseBotDifficulties(doc, rocketPlugin->botDifficulties);
        parseAvailableMaps(doc, rocketPlugin->maps);
        parseAvailableColors(
            doc, rocketPlugin->customColorHues, rocketPlugin->customColors, rocketPlugin->clubColorHues,
            rocketPlugin->clubColors, rocketPlugin->defaultBluePrimaryColor, rocketPlugin->defaultBlueAccentColor,
            rocketPlugin->defaultOrangePrimaryColor, rocketPlugin->defaultOrangeAccentColor);
        parseAvailableMutators(doc, rocketPlugin->mutators);
    }
    catch (const simdjson::simdjson_error& e) {
        BM_CRITICAL_LOG("failed to parse game settings, {:s}", quote(e.what()));
        return false;
    } catch (const std::exception& e) {
        BM_CRITICAL_LOG("failed to parse game settings, {:s}", quote(e.what()));
        return false;
    } catch (...) {
        BM_CRITICAL_LOG("failed to parse game settings");
        return false;
    }

    return true;
}


/// <summary>Parses rumble items json string into Crazy Rumble variables.</summary>
/// <param name="data">Json string with rumble item settings</param>
/// <param name="crazyRumble">Crazy Rumble class to store the result in</param>
/// <returns>Bool with if the rumble item settings where parsed successfully</returns>
bool RPConfig::ParseRumbleItems([[maybe_unused]] const std::string& data, [[maybe_unused]] CrazyRumble* crazyRumble)
{
    return false;
}


/// <summary>Request the game setting constants.</summary>
/// <returns>Future with the game setting constants</returns>
std::future<std::pair<bool, std::string>> RPConfig::RequestGameSettingConstants()
{
    return std::async([this]() {
        if (gameSettingConstantsConfigUrl.empty()) {
            if (!requestConfig().get()) {
                BM_ERROR_LOG("config request failed");
                return std::pair(false, std::string());
            }
        }

        return Request(gameSettingConstantsConfigUrl).get();
    });
}


/// <summary>Request the rumble constants.</summary>
/// <returns>Future with the rumble constants</returns>
std::future<std::pair<bool, std::string>> RPConfig::RequestRumbleConstants()
{
    return std::async([this]() {
        if (rumbleConstantsConfigUrl.empty()) {
            if (!requestConfig().get()) {
                BM_ERROR_LOG("config request failed");
                return std::pair(false, std::string());
            }
        }

        return Request(rumbleConstantsConfigUrl).get();
    });
}


/// <summary>Parses game settings from json object.</summary>
/// <param name="gameSettingJson">Json object with game settings</param>
/// <returns>Parsed game settings</returns>
RocketPlugin::GameSetting ParseGameSetting(simdjson::ondemand::object gameSettingJson)
{
    RocketPlugin::GameSetting gameSetting;
    gameSetting.DisplayCategoryName  = std::string_view(gameSettingJson["DisplayCategoryName"]);
    gameSetting.InternalCategoryName = std::string_view(gameSettingJson["InternalCategoryName"]);
    for (simdjson::ondemand::value name : gameSettingJson["DisplayName"].get_array()) {
        gameSetting.DisplayName.emplace_back(std::string_view(name));
    }
    for (simdjson::ondemand::value name : gameSettingJson["InternalName"].get_array()) {
        gameSetting.InternalName.emplace_back(std::string_view(name));
    }

    return gameSetting;
}


/// <summary>Parses game settings from json object.</summary>
/// <param name="doc">Json object with game settings</param>
/// <param name="gameModes">Game setting to save the parsed game settings to</param>
void RPConfig::parseGameModes(simdjson::ondemand::document& doc, RocketPlugin::GameSetting& gameModes)
{
    gameModes = ParseGameSetting(doc["GameModes"]);
}


/// <summary>Parses bot difficulties from json object.</summary>
/// <param name="doc">Json object with bot difficulties</param>
/// <param name="botDifficulties">Game setting to save the parsed bot difficulties to</param>
void RPConfig::parseBotDifficulties(simdjson::ondemand::document& doc, RocketPlugin::GameSetting& botDifficulties)
{
    botDifficulties = ParseGameSetting(doc["BotDifficulties"]);
}


/// <summary>Parses available maps from json object.</summary>
/// <param name="doc">Json object with available maps</param>
/// <param name="maps">Map to save the parsed available maps to</param>
void RPConfig::parseAvailableMaps(simdjson::ondemand::document& doc, std::map<std::string, std::string>& maps)
{
    maps.clear();
    for (simdjson::ondemand::field field : doc["Maps"].get_object()) {
        maps.emplace(std::string_view(field.unescaped_key()), std::string_view(field.value()));
    }
}


/// <summary>Parses color from json object.</summary>
/// <param name="arr">Json array with color</param>
/// <returns>Parsed color</returns>
ImVec4 ParseColor(simdjson::ondemand::array arr)
{
    ImVec4 color;
    simdjson::ondemand::array_iterator it = arr.begin();
    color.x = static_cast<float>(static_cast<double>(*it));
    ++it;
    color.y = static_cast<float>(static_cast<double>(*it));
    ++it;
    color.z = static_cast<float>(static_cast<double>(*it));
    color.w = 1;

    return color;
}

/// <summary>Parses available colors from json object.</summary>
/// <param name="doc">Json object with available colors</param>
/// <param name="customColorHues">Int to save the parsed number of custom color hues to</param>
/// <param name="customColors">Vector to save the parsed custom colors to</param>
/// <param name="clubColorHues">Int to save the parsed number of club color hues to</param>
/// <param name="clubColors">Vector to save the parsed club colors to</param>
/// <param name="bluePrimaryColor">Color to save the parsed blue primary color to</param>
/// <param name="blueAccentColor">Color to save the parsed blue accent color to</param>
/// <param name="orangePrimaryColor">Color to save the parsed orange primary color to</param>
/// <param name="orangeAccentColor">Color to save the parsed orange accent color to</param>
void RPConfig::parseAvailableColors(simdjson::ondemand::document& doc, int& customColorHues,
    std::vector<ImVec4>& customColors, int& clubColorHues, std::vector<ImVec4>& clubColors, ImVec4& bluePrimaryColor,
    ImVec4& blueAccentColor, ImVec4& orangePrimaryColor, ImVec4& orangeAccentColor)
{
    customColorHues = static_cast<int>(doc["CustomColorHues"].get_uint64());
    customColors.clear();
    for (simdjson::ondemand::value value : doc["CustomColors"]) {
        customColors.push_back(ParseColor(value));
    }
    clubColorHues = static_cast<int>(doc["ClubColorHues"].get_uint64());
    clubColors.clear();
    for (simdjson::ondemand::value value : doc["ClubColors"]) {
        clubColors.push_back(ParseColor(value));
    }

    bluePrimaryColor   = ParseColor(doc["DefaultBluePrimaryColor"]);
    blueAccentColor    = ParseColor(doc["DefaultBlueAccentColor"]);
    orangePrimaryColor = ParseColor(doc["DefaultOrangePrimaryColor"]);
    orangeAccentColor  = ParseColor(doc["DefaultOrangeAccentColor"]);
}


/// <summary>Parses available mutators from json object.</summary>
/// <param name="doc">Json object with available mutators</param>
/// <param name="mutators">Vector to save the parsed available mutators to</param>
void RPConfig::parseAvailableMutators(simdjson::ondemand::document& doc,
    std::vector<RocketPlugin::GameSetting>& mutators)
{
    mutators.clear();
    for (simdjson::ondemand::value value : doc["Mutators"]) {
        mutators.push_back(ParseGameSetting(value));
    }
}


/// <summary>Requests and parses Rocket Plugin config.</summary>
/// <returns>Future with if the request was successful</returns>
std::future<bool> RPConfig::requestConfig()
{
    return save_promise<bool>("", [this]() {
        const auto& [requestSuccessful, responseData] = Request(configUrl).get();
        if (!requestSuccessful) {
            BM_ERROR_LOG("config request failed");
            return false;
        }

        simdjson::dom::parser parser;
        const simdjson::dom::element dom = parser.parse(responseData);

        bool failed = false;
        std::string_view strView;
        simdjson::error_code error = dom["RLConstants"].get(strView);
        if (error) {
            failed = true;
            BM_ERROR_LOG("could not get RLConstants, {:s}", simdjson::error_message(error));
        }
        else {
            gameSettingConstantsConfigUrl = strView;
        }

        error = dom["RumbleConstants"].get(strView);
        if (error) {
            failed = true;
            BM_ERROR_LOG("could not get RumbleConstants, {:s}", simdjson::error_message(error));
        }
        else {
            rumbleConstantsConfigUrl = strView;
        }

        return !failed;
    });
}
