// RocketPluginGUI.cpp
// GUI for the RocketPlugin plugin.
//
// Author:        Stanbroek
// Version:       0.6.6 16/04/21
// BMSDK version: 95

#include "Config.h"
#include "RocketPlugin.h"
#include "GameModes/RocketGameMode.h"

#include "ImGui/imgui_internal.h"

#define IM_COL32_ERROR        (ImColor(204,   0,   0, 255))
#define IM_COL32_WARNING      (ImColor(255,  60,   0,  80))
#define IM_COL32_ERROR_BANNER (ImColor(211,  47,  47, 255))


/*
 *  Plugin window overrides
 */

/// <summary>ImGui widgets to rendered every tick.</summary>
void RocketPlugin::OnRender()
{
    if (refreshGameSettingsConstants) {
        if (!gameSettingsRequest.valid()) {
            gameSettingsRequest = ConstantsConfig->RequestGameSettingConstants();
        }
        else if (gameSettingsRequest._Is_ready()) {
            refreshGameSettingsConstants = false;
            const auto& [successful, data] = gameSettingsRequest.get();
            if (!successful || !RPConfig::ParseGameSettings(data, this)) {
                Execute([this](GameWrapper*) {
                    WARNING_LOG("could not load game settings");
                    loadRLConstants();
                });
            }
#ifdef DEBUG
            else {
                Execute([this](GameWrapper*) {
                    TRACE_LOG("loading game settings from game files.");
                    loadRLConstants();
                });
            }
#endif
        }
    }

#ifdef DEBUG
    IMGUI_CHECKVERSION();
    if (*showDemoWindow) {
        ImGui::ShowDemoWindow(showDemoWindow.get());
    }
    if (*showMetricsWindow) {
        ImGui::ShowMetricsWindow(showMetricsWindow.get());
    }
#endif

    ImGui::SetNextWindowSizeConstraints(ImVec2(800, 600), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin(menuTitle.c_str(), &isWindowOpen)) {
        if (ImGui::BeginTabBar("#TabBar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_NoTooltip)) {
            renderMultiplayerTab();
            renderInGameModsTab();
            renderGameModesTab();
            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    if (!isWindowOpen) {
        cvarManager->executeCommand("togglemenu " + GetMenuName());
    }
}


/// <summary>Gets the menu name.</summary>
/// <returns>The menu name</returns>
std::string RocketPlugin::GetMenuName()
{
    return "rocketplugin";
}


/// <summary>Gets the menu title.</summary>
/// <returns>The menu title</returns>
std::string RocketPlugin::GetMenuTitle()
{
    return menuTitle;
}


/// <summary>Sets the ImGui context.</summary>
void RocketPlugin::SetImGuiContext(const uintptr_t ctx)
{
    ImGuiContext* context = reinterpret_cast<ImGuiContext*>(ctx);
    ImGui::SetCurrentContext(context);
}


/// <summary>Gets if the user input should be blocked.</summary>
/// <remarks>This returns true when the user is using the widgets.</remarks>
/// <returns>Bool with if the user input should be blocked</returns>
bool RocketPlugin::ShouldBlockInput()
{
    return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}


/// <summary>Gets if it is an active overlay.</summary>
/// <remarks>This always returns true.</remarks>
/// <returns>Bool with if it is an active overlay</returns>
bool RocketPlugin::IsActiveOverlay()
{
    return true;
}


/// <summary>Sets the window to open.</summary>
void RocketPlugin::OnOpen()
{
    isWindowOpen = true;
}


/// <summary>Sets the window to close.</summary>
void RocketPlugin::OnClose()
{
    isWindowOpen = false;
}


/*
 *  General settings
 */

/// <summary>Pushes an error message to be displayed in the GUI.</summary>
/// <param name="message">Message to be displayed in the GUI</param>
void RocketPlugin::PushError(const std::string& message)
{
    errors.push(message);
}


/// <summary>Renders the multiplayer tab.</summary>
void RocketPlugin::renderMultiplayerTab()
{
    if (ImGui::BeginTabItem("Multiplayer")) {
        if (!errors.empty()) {
            if (ImGui::Banner(errors.front().c_str(), IM_COL32_ERROR_BANNER)) {
                errors.pop();
            }
        }
        renderMultiplayerTabHost();
        ImGui::SameLine();
        renderMultiplayerTabJoin();
        ImGui::EndTabItem();
    }
}


/// <summary>Renders the custom map selection widget.</summary>
/// <param name="customMaps">Map of internal map names to display names</param>
/// <param name="currentCustomMap">Internal map name of the currently selected map</param>
/// <param name="refreshCustomMaps">Bool with if maps needs to be refreshed, is also set when the custom map selection widget is closed</param>
/// <param name="includeWorkshopMaps">Bool with if to include maps from the workshop maps directory, default to true</param>
/// <param name="includeCustomMaps">Bool with if to include maps from the custom maps directory, default to true</param>
/// <returns>Bool with if a custom map was selected</returns>
bool RocketPlugin::renderCustomMapsSelection(std::map<std::filesystem::path, std::string>& customMaps,
    std::filesystem::path& currentCustomMap, bool& refreshCustomMaps,
    const bool includeWorkshopMaps, const bool includeCustomMaps)
{
    if (refreshCustomMaps) {
        customMaps.clear();
        if (includeWorkshopMaps && gameWrapper->IsUsingSteamVersion()) {
            bool addedWorkshopMaps = false;
            customMaps.insert({ *workshopMapDirPath, "#Workshop Maps:" });
            for (const std::filesystem::path& workshopMap : getWorkshopMaps(*workshopMapDirPath)) {
                addedWorkshopMaps = true;
                if (auto it = subscribedWorkshopMaps.find(
                    std::strtoull(workshopMap.parent_path().stem().string().c_str(), nullptr, 10));
                    it != subscribedWorkshopMaps.end() && !it->second.Title.empty()) {
                    customMaps.insert({ workshopMap, it->second.Title });
                }
                else {
                    customMaps.insert({ workshopMap, workshopMap.stem().u8string() });
                }
            }
            if (!addedWorkshopMaps) {
                customMaps.erase(*workshopMapDirPath);
            }
        }
        if (includeCustomMaps) {
            bool addedCustomMaps = false;
            customMaps.insert({ *customMapDirPath, "#Custom Maps:" });
            for (const std::filesystem::path& customMap : GetFilesFromDir(*customMapDirPath, 2, ".upk", ".udk")) {
                addedCustomMaps = true;
                if (auto it = subscribedWorkshopMaps.find(
                    std::strtoull(customMap.parent_path().stem().string().c_str(), nullptr, 10));
                    it != subscribedWorkshopMaps.end() && !it->second.Title.empty()) {
                    customMaps.insert({ customMap, it->second.Title });
                }
                else {
                    customMaps.insert({ customMap, customMap.stem().u8string() });
                }
            }
            if (!addedCustomMaps) {
                customMaps.erase(*customMapDirPath);
            }
            bool addedCopiedMaps = false;
            customMaps.insert({ COPIED_MAPS_PATH, "#Copied Maps:" });
            for (const std::filesystem::path& customMap : GetFilesFromDir(COPIED_MAPS_PATH, 1, ".upk")) {
                addedCopiedMaps = true;
                customMaps.insert({ customMap, customMap.stem().u8string() });
            }
            if (!addedCopiedMaps) {
                customMaps.erase(COPIED_MAPS_PATH);
            }
        }
        refreshCustomMaps = false;
    }

    bool valueChanged = false;
    if (ImGui::SearchableCombo("##Maps", currentCustomMap, customMaps, "No maps found", "type to search")) {
        valueChanged = true;
        if (!ImGui::IsItemActiveLastFrame()) {
            refreshCustomMaps = true;
        }
    }

    return valueChanged;
}


/*
 *  Host settings
 */

/// <summary>Renders the host section in game multiplayer tab.</summary>
void RocketPlugin::renderMultiplayerTabHost()
{
    if (ImGui::BeginChild("#HostGame", ImVec2(-ImGui::GetFrameWidthWithSpacing() * 6, -ImGui::GetFrameHeightWithSpacing() + 23), true)) {
        ImGui::Indent(5);
        ImGui::Spacing();

        ImGui::TextUnformatted("Host a local game");
        ImGui::Separator();

        ImGui::TextUnformatted(" Game Mode:");
        ImGui::Combo("##GameMode", &gameModes.CurrentSelected, gameModes.DisplayName, "No maps found");
        ImGui::TextUnformatted(" Arena:");
        if (gameWrapper->IsUsingSteamVersion()) {
            ImGui::SameLine();
            if (ImGui::Checkbox("Enable workshop maps", &enableWorkshopMaps)) {
                currentMap.clear();
                refreshCustomMapPaths = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Enable custom maps", &enableCustomMaps)) {
            currentMap.clear();
            refreshCustomMapPaths = true;
        }
        if (!enableWorkshopMaps && !enableCustomMaps) {
            const bool currentMapIsModdedCache = currentMapIsModded;
            if (currentMapIsModdedCache) {
                ImGui::BeginWarnBorder();
            }
            if (ImGui::SearchableCombo("##Maps", currentMap, maps, "No maps found", "type to search")) {
                gameWrapper->Execute([this](GameWrapper*) {
                    currentMapIsModded = isCurrentMapModded();
                });
            }
            if (currentMapIsModdedCache) {
                ImGui::EndWarnBorder();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Replacing official maps can get you banned from matchmaking.");
                }
            }
        }
        else {
            std::filesystem::path currentMapPath = currentMap;
            if (renderCustomMapsSelection(customMapPaths, currentMapPath, refreshCustomMapPaths, enableWorkshopMaps, enableCustomMaps)) {
                currentMap = currentMapPath.string();
                const std::filesystem::path config = currentMapPath.replace_extension(L".cfg");
                if (exists(config)) {
                    TRACE_LOG("loading map config");
                    loadPreset(config.string());
                }
                else {
                    TRACE_LOG("could not find map config");
                }
            }
        }
        ImGui::TextUnformatted(" Player count:");
        if (ImGui::SliderInt("##PlayerCount", &playerCount, 2, 8, "%d players") && playerCount < 2) {
            playerCount = 6;
        }
        ImGui::TextUnformatted(" Bot Difficulty:");
        ImGui::SliderArray("##BotDifficulty", &botDifficulties.CurrentSelected, botDifficulties.DisplayName);
        if (ImGui::CollapsingHeader("Team Settings")) {
            ImGui::TextUnformatted("Team 1");
            ImGui::BeginColumns("Team 1", 2, ImGuiColumnsFlags_NoBorder);
            {
                ImGui::SetColumnWidth(0, 100);
                ImGui::SetCursorPosX(32);
                ImGui::TextUnformatted("Team Name:");
                ImGui::NextColumn();

                ImGui::SetNextItemWidth(100);
                ImGui::InputTextWithHint("##Team1Name", "BLUE", &team1Name, ImGuiInputTextFlags_CharsUppercase);
                ImGui::NextColumn();

                ImGui::SetCursorPosX(20);
                ImGui::TextUnformatted("Primary Color:");
                ImGui::NextColumn();

                ImGui::RLColorPicker("##Team1PrimaryColor", &team1PrimCol, clubColors, clubColorHues, defaultBluePrimaryColor, ImVec2(100, 0));
                ImGui::NextColumn();

                ImGui::SetCursorPosX(28);
                ImGui::TextUnformatted("Accent Color:");
                ImGui::NextColumn();

                ImGui::RLColorPicker("##Team1AccentColor", &team1AccCol, customColors, customColorHues, defaultBlueAccentColor, ImVec2(100, 0));
            }
            ImGui::EndColumns();
            ImGui::TextUnformatted("Team 2");
            ImGui::BeginColumns("Team 2", 2, ImGuiColumnsFlags_NoBorder);
            {
                ImGui::SetColumnWidth(0, 100);
                ImGui::SetCursorPosX(32);
                ImGui::TextUnformatted("Team Name:");
                ImGui::NextColumn();

                ImGui::SetNextItemWidth(100);
                ImGui::InputTextWithHint("##Team2Name", "ORANGE", &team2Name, ImGuiInputTextFlags_CharsUppercase);
                ImGui::NextColumn();

                ImGui::SetCursorPosX(20);
                ImGui::TextUnformatted("Primary Color:");
                ImGui::NextColumn();

                ImGui::RLColorPicker("##Team2PrimaryColor", &team2PrimCol, clubColors, clubColorHues, defaultOrangePrimaryColor, ImVec2(100, 0));
                ImGui::NextColumn();

                ImGui::SetCursorPosX(28);
                ImGui::TextUnformatted("Accent Color:");
                ImGui::NextColumn();

                ImGui::RLColorPicker("##Team2AccentColor", &team2AccCol, customColors, customColorHues, defaultOrangeAccentColor, ImVec2(100, 0));
            }
            ImGui::EndColumns();
            ImGui::Checkbox("Club Match", &clubMatch);
        }
        if (ImGui::CollapsingHeader("Mutators Settings")) {
            if (ImGui::BeginChild("#MutatorsSettings", ImVec2(-ImGui::GetFrameWidthWithSpacing() * 4, static_cast<float>(mutators.size()) * 23.f), false, ImGuiWindowFlags_NoScrollbar)) {
                ImGui::BeginColumns("Mutators", 2, ImGuiColumnsFlags_NoBorder);
                {
                    static float columnWidth = 125;
                    ImGui::SetColumnWidth(0, columnWidth + ImGui::GetStyle().ItemSpacing.x);
                    float maxColumnWidth = 0;
                    for (GameSetting& mutator : mutators) {
                        const std::string displayName = std::string(mutator.DisplayCategoryName) + ":";
                        const ImVec2 size = ImGui::CalcTextSize(displayName.c_str());
                        if (size.x > maxColumnWidth) {
                            maxColumnWidth = size.x;
                        }
                        ImGui::SetCursorPosX(columnWidth - size.x + ImGui::GetStyle().ItemSpacing.x);
                        ImGui::TextUnformatted(displayName.c_str());
                        ImGui::NextColumn();

                        const std::string label = "##" + mutator.InternalCategoryName;
                        ImGui::PushItemWidth(ImGui::GetWindowWidth() / 3 * 2);
                        ImGui::SliderArray(label.c_str(), &mutator.CurrentSelected, mutator.DisplayName);
                        ImGui::NextColumn();
                    }
                    if (maxColumnWidth > columnWidth) {
                        columnWidth = maxColumnWidth;
                    }
                }
                ImGui::EndColumns();
            }
            ImGui::EndChild();
            ImGui::SameLine();
            if (ImGui::BeginChild("#MutatorsPresets", ImVec2(0.f, static_cast<float>(mutators.size()) * 23.f))) {
                ImGui::TextUnformatted(" Presets:");
                ImGui::Separator();

                if (ImGui::Button("Default", ImVec2(-5, 0))) {
                    resetMutators();
                }
                for (const std::filesystem::path& presetPath : presetPaths) {
                    std::string filename = presetPath.filename().string();
                    if (ImGui::Button(filename.substr(0, filename.size() - 4).c_str(), ImVec2(-5, 0))) {
                        loadPreset(presetPath);
                    }
                }
                if (presetPaths.empty()) {
                    ImGui::PushItemWidth(-5);
                    ImGui::TextUnformatted("No presets found.");
                }
                ImGui::Separator();

                if (ImGui::Button("Save preset")) {
                    ImGui::OpenPopup("Save preset as");
                }
                if (ImGui::BeginPopup("Save preset as")) {
                    static char nameBuf[16] = "";
                    ImGui::InputText(".cfg", nameBuf, 16);
                    if (ImGui::Button("Save")) {
                        savePreset(std::string(nameBuf) + ".cfg");
                        presetPaths = GetFilesFromDir(*presetDirPath, 1, ".cfg");
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::EndChild();
        }
        if (ImGui::CollapsingHeader("Advanced Settings")) {
            ImGui::TextUnformatted(" Password: (optional)");
            ImGui::InputText("##pswd_host", &hostPswd, ImGuiInputTextFlags_Password);
            ImGui::TextUnformatted((" Internal host port: (default is " + std::to_string(DEFAULT_PORT) + ")").c_str());
            // TODO, Allow changing the internal host port.
            ImGui::BeginDisabled();
            const bool invalidHostPortInternal = !Networking::IsValidPort(hostPortInternal);
            if (invalidHostPortInternal) {
                ImGui::BeginErrorBorder();
                ImGui::InputScalar("##HostPortInternal", ImGuiDataType_U16, &hostPortInternal);
                ImGui::EndErrorBorder();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Invalid port");
                }
            }
            else {
                ImGui::InputScalar("##HostPortInternal", ImGuiDataType_U16, &hostPortInternal);
            }
            ImGui::EndDisabled();
            ImGui::TextUnformatted(" Network options:");
            ImGui::Indent(4);
            const bool upnpFailed = upnpClient == nullptr;
            if (upnpFailed) {
                // Disables the collapsing header.
                ImGui::BeginDisabled();
            }
            if (ImGui::CollapsingHeader("UPnP Settings")) {
                ImGui::Indent(10);
                ImGui::PushStyleColor(ImGuiCol_Border, static_cast<ImVec4>(IM_COL32_WARNING));
                if (ImGui::BeginChild("##UPnPWarning", ImVec2(0, 50), true)) {
                    ImGui::Dummy(ImVec2(140.0f, 0.0f));
                    ImGui::SameLine();
                    ImGui::TextUnformatted("WARNING these are advanced settings!");
                    ImGui::TextUnformatted("Using these without knowing what your are doing could put your computer and network at risk.");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                if (ImGui::Button("Search for UPnP devices")) {
                    if (!upnpClient->DiscoverySearching()) {
                        Execute([this](GameWrapper*) {
                            upnpClient->FindDevices();
                        });
                    }
                    else {
                        WARNING_LOG("Already searching for UPnP devices");
                    }
                }
                ImGui::SameLine();
                if (upnpClient->DiscoveryFailed()) {
                    ImGui::TextColoredWrapped(IM_COL32_ERROR, upnpClient->GetDiscoveryStatus().c_str());
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Make sure your router supports UPnP \nand has it enabled it the settings.");
                    }
                }
                else {
                    ImGui::TextWrapped(upnpClient->GetDiscoveryStatus());
                }
                const bool invalidHostPortExternal = !Networking::IsValidPort(hostPortExternal);
                if (upnpClient->DiscoveryFinished()) {
                    if (ImGui::Button(("Forward port " + (invalidHostPortExternal ? std::to_string(DEFAULT_PORT) : std::to_string(hostPortExternal))).c_str())) {
                        if (!upnpClient->ServiceForwardPortActive()) {
                            Execute([this, invalidHostPortExternal](GameWrapper*) {
                                upnpClient->ForwardPort(invalidHostPortExternal ? DEFAULT_PORT : hostPortInternal,
                                    invalidHostPortExternal ? DEFAULT_PORT : hostPortExternal, portLeaseDuration);
                            });
                        }
                        else {
                            WARNING_LOG("Already forwarding a port");
                        }
                    }
                    if (upnpClient->ServiceForwardPortFailed()) {
                        ImGui::SameLine();
                        ImGui::TextColoredWrapped(IM_COL32_ERROR, upnpClient->GetForwardPortStatus().c_str());
                    }
                    else if (upnpClient->ServiceForwardPortFinished()) {
                        ImGui::SameLine();
                        ImGui::TextWrapped(upnpClient->GetForwardPortStatus());
                    }
                }
                ImGui::TextUnformatted("External Address:");
                const float itemWidth = std::max(0.f, ImGui::GetCurrentWindowRead()->DC.ItemWidth - ImGui::CalcTextSize(":").x);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
                ImGui::PushItemWidth(itemWidth / 3 * 2);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                ImGui::InputText("##ExternalIPAddress", upnpClient->GetExternalIpAddressBuffer(), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                ImGui::PopStyleVar();
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::TextUnformatted(":");
                ImGui::SameLine();
                ImGui::PushItemWidth(itemWidth / 3);
                if (invalidHostPortExternal) {
                    ImGui::BeginErrorBorder();
                    ImGui::InputScalar("##UPnPHostPortExternal", ImGuiDataType_U16, &hostPortExternal);
                    ImGui::EndErrorBorder();
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Invalid port");
                    }
                }
                else {
                    ImGui::InputScalar("##UPnPHostPortExternal", ImGuiDataType_U16, &hostPortExternal);
                }
                ImGui::PopItemWidth();
                ImGui::PopStyleVar();
                ImGui::TextUnformatted("Duration to open your port for: (0 means indefinite)");
                constexpr int weekDuration = 60 * 60 * 24 * 7;
                ImGui::DragTime(" hour:minutes:seconds##portLeaseDuration", &portLeaseDuration, 60, 0, weekDuration);
                if (upnpClient->DiscoveryFinished()) {
                    ImGui::TextUnformatted("Open ports:");
                    ImGui::SameLine();
                    if (upnpClient->ServiceClosePortFailed()) {
                        ImGui::TextColoredWrapped(IM_COL32_ERROR, upnpClient->GetClosePortStatus().c_str());
                    }
                    else if (upnpClient->ServiceClosePortFinished()) {
                        ImGui::TextWrapped(upnpClient->GetClosePortStatus());
                    }
                    const std::vector<unsigned short>& openPorts = upnpClient->GetOpenPorts();
                    if (openPorts.empty()) {
                        ImGui::TextUnformatted("\tNo open ports found.");
                    }
                    else {
                        for (const unsigned short openPort : openPorts) {
                            if (ImGui::Button(std::to_string(openPort).c_str())) {
                                if (!upnpClient->ServiceClosePortActive()) {
                                    Execute([this, openPort](GameWrapper*) {
                                        upnpClient->ClosePort(openPort);
                                    });
                                }
                                else {
                                    WARNING_LOG("Already closing a port");
                                }
                            }
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("Close port " + std::to_string(openPort) + ".");
                            }
                            ImGui::SameLine();
                        }
                        ImGui::NewLine();
                    }
                }
                ImGui::Unindent(10);
            }
            if (upnpFailed) {
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Failed to create networking thread.");
                }
            }
            const bool p2pFailed = p2pHost == nullptr;
            if (p2pFailed) {
                // Disables the collapsing header.
                ImGui::BeginDisabled();
            }
            if (ImGui::CollapsingHeader("P2P Settings")) {
                ImGui::Indent(10);
                ImGui::PushStyleColor(ImGuiCol_Border, static_cast<ImVec4>(IM_COL32_WARNING));
                if (ImGui::BeginChild("##P2PWarning", ImVec2(0, 50), true)) {
                    ImGui::Dummy(ImVec2(140.0f, 0.0f));
                    ImGui::SameLine();
                    ImGui::TextUnformatted("WARNING these are advanced settings!");
                    ImGui::TextUnformatted("Using these without knowing what your are doing could put your computer and network at risk.");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                if (ImGui::Button("get NAT type")) {
                    if (p2pHost->GetNATType() != P2PHost::NATType::NAT_SEARCHING) {
                        Execute([this](GameWrapper*) {
                            p2pHost->FindNATType(hostPortInternal);
                        });
                    }
                    else {
                        WARNING_LOG("Already getting NAT type.");
                    }
                }
                ImGui::SameLine();
                ImGui::TextWrapped(p2pHost->GetNATDesc());
                const bool invalidHostPortExternal = !Networking::IsValidPort(hostPortExternal);
                if (p2pHost->GetNATType() == P2PHost::NATType::NAT_FULL_CONE || p2pHost->GetNATType() == P2PHost::NATType::NAT_RESTRICTED_PORT) {
                    ImGui::TextUnformatted((" External host port: (default is " + std::to_string(DEFAULT_PORT) + ")").c_str());
                    if (invalidHostPortExternal) {
                        ImGui::BeginErrorBorder();
                        ImGui::InputScalar("##NATHostPortExternal", ImGuiDataType_U16, &hostPortExternal);
                        ImGui::EndErrorBorder();
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Invalid port");
                        }
                    }
                    else {
                        ImGui::InputScalar("##NATHostPortExternal", ImGuiDataType_U16, &hostPortExternal);
                    }
                }
                switch (p2pHost->GetNATType()) {
                case P2PHost::NATType::NAT_FULL_CONE:
                    if (ImGui::Button(("Punch through port " + std::to_string(invalidHostPortExternal ? DEFAULT_PORT : hostPortExternal)).c_str())) {
                        Execute([this, invalidHostPortExternal](GameWrapper*) {
                            p2pHost->PunchPort(NAT_PUNCH_ADDR, invalidHostPortExternal ? DEFAULT_PORT : hostPortExternal);
                        });
                    }
                    break;
                case P2PHost::NATType::NAT_RESTRICTED_PORT:
                    if (connections.size() > 1 && connections[connections.size() - 1].IP[0] == '\0' && connections[connections.size() - 2].IP[0] == '\0') {
                        connections.pop_back();
                    }
                    if (connections.empty() || connections.back().IP[0] != '\0') {
                        connections.push_back({ { '\0' }, true });
                    }
                    for (size_t i = 0; i < connections.size(); i++) {
                        if (connections[i].InvalidIP) {
                            ImGui::BeginErrorBorder();
                            if (ImGui::InputText(("##P2PClientIP" + std::to_string(i)).c_str(), &connections[i].IP[0], sizeof P2PIP::IP)) {
                                connections[i].InvalidIP = !Networking::IsValidIPv4(connections[i].IP);
                            }
                            ImGui::EndErrorBorder();
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("Invalid ipv4");
                            }
                        }
                        else {
                            if (ImGui::InputText(("##P2PClientIP" + std::to_string(i)).c_str(), &connections[i].IP[0], sizeof P2PIP::IP)) {
                                connections[i].InvalidIP = !Networking::IsValidIPv4(connections[i].IP);
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(("Start Connection##P2PClientConn" + std::to_string(i)).c_str())) {
                            p2pHost->PunchPort(connections[i].IP, hostPortExternal);
                            Execute([this, addr = connections[i].IP](GameWrapper*) {
                                p2pHost->PunchPort(addr, hostPortExternal);
                            });
                        }
                    }
                    break;
                default:
                    break;
                }
                ImGui::Unindent(10);
            }
            if (p2pFailed) {
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Failed to create networking thread.");
                }
            }
            ImGui::Unindent(4);
            ImGui::Checkbox("Host game with party", &hostWithParty);
            ImGui::SameLine();
            ImGui::TextUnformatted("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Will send a party game invite to everyone in your party.\nThis only works when connecting directly to your IP.");
            }
        }
        ImGui::Separator();

        if (isHostingLocalGame()) {
            if (ImGui::Button("Host New Match")) {
                Execute([this](GameWrapper*) {
                    HostGame();
                });
            }
            ImGui::SameLine();
            if (ImGui::Button("Update Match Settings")) {
                Execute([this](GameWrapper*) {
                    if (enableWorkshopMaps || enableCustomMaps) {
                        const std::filesystem::path map = currentMap;
                        preLoadMap(std::filesystem::absolute(map));
                        setMatchSettings(map.stem().string());
                    }
                    else {
                        setMatchSettings(currentMap);
                    }
                    TRACE_LOG("updated match settings");
                });
            }
        }
        else {
            if (ImGui::Button("Host")) {
                Execute([this](GameWrapper*) {
                    HostGame();
                });
            }
        }
    }
    ImGui::EndChild();
}


/*
 *  Join settings
 */

/// <summary>Begins host status border block.</summary>
/// <param name="addrType">Address type to determine the status border</param>
/// <param name="hostStatus">Host status to determine the status border</param>
void BeginHostStatusBorder(const Networking::DestAddrType addrType, const Networking::HostStatus hostStatus)
{
    if (addrType == Networking::DestAddrType::UNKNOWN_ADDR || hostStatus == Networking::HostStatus::HOST_ERROR) {
        ImGui::BeginErrorBorder();
    }
    else if (addrType == Networking::DestAddrType::PRIVATE_ADDR || addrType == Networking::DestAddrType::INTERNL_ADDR ||
        hostStatus == Networking::HostStatus::HOST_TIMEOUT) {
        ImGui::BeginWarnBorder();
    }
    else if (hostStatus == Networking::HostStatus::HOST_ONLINE) {
        ImGui::BeginSuccessBorder();
    }
    else {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, ImGui::GetStyle().FrameBorderSize);
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyleColorVec4(ImGuiCol_Border));
        ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImGui::GetStyleColorVec4(ImGuiCol_BorderShadow));
    }
}


/// <summary>Renders the join section in game multiplayer tab.</summary>
void RocketPlugin::renderMultiplayerTabJoin()
{
    if (ImGui::BeginChild("#JoinGame", ImVec2(0, 0), true)) {
        ImGui::PushItemWidth(-5);
        ImGui::Indent(5);
        ImGui::Spacing();

        ImGui::TextUnformatted("Join a local game");
        ImGui::Separator();

        ImGui::TextUnformatted(" IP Address:");
        static Networking::HostStatus hostOnline = Networking::HostStatus::HOST_UNKNOWN;
        static Networking::DestAddrType addressType = Networking::GetDestAddrType(joinIP->c_str());
        BeginHostStatusBorder(addressType, hostOnline);
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
        if (hostOnline == Networking::HostStatus::HOST_BUSY) {
            flags |= ImGuiInputTextFlags_ReadOnly;
        }
        if (ImGui::InputText("##ip_join", joinIP.get(), flags)) {
            cvarManager->getCvar("mp_ip").setValue(*joinIP);
            addressType = Networking::GetDestAddrType(joinIP->c_str());
            if (hostOnline != Networking::HostStatus::HOST_BUSY) {
                hostOnline = Networking::HostStatus::HOST_UNKNOWN;
            }
        }
        ImGui::EndBorder();
        if (hostOnline == Networking::HostStatus::HOST_UNKNOWN && addressType != Networking::DestAddrType::UNKNOWN_ADDR) {
            // Only ping when the user is finished typing.
            if (ImGui::IsItemActiveLastFrame() && !ImGui::IsItemActive()) {
                Networking::PingHost(*joinIP, static_cast<unsigned short>(*joinPort), &hostOnline, true);
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(Networking::GetHostStatusHint(addressType, hostOnline));
        }
        ImGui::TextUnformatted(" Port:");
        const bool invalidJoinPort = !Networking::IsValidPort(*joinPort);
        if (invalidJoinPort) {
            ImGui::BeginErrorBorder();
            if (ImGui::InputScalar("##port_join", ImGuiDataType_U16, joinPort.get())) {
                cvarManager->getCvar("mp_port").setValue(*joinPort);
            }
            ImGui::EndErrorBorder();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Invalid port");
            }
        }
        else {
            if (ImGui::InputScalar("##port_join", ImGuiDataType_U16, joinPort.get())) {
                cvarManager->getCvar("mp_port").setValue(*joinPort);
            }
        }
        ImGui::TextUnformatted(" Password: (optional)");
        static char pswdBuf[64] = "";
        ImGui::InputText("##pswd_join", pswdBuf, 64, ImGuiInputTextFlags_Password);
        ImGui::Separator();

        ImGui::Unindent(5);
        static bool isCurrentMapJoinable = true;
        if (ImGui::Checkbox("Joining a custom map", &joinCustomMap)) {
            if (joinCustomMap) {
                Execute([this](GameWrapper*) {
                    isCurrentMapJoinable = isMapJoinable(currentJoinMap);
                });
            }
            else {
                isCurrentMapJoinable = true;
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Select this if you are joining a non Rocket League map.");
        }
        if (joinCustomMap) {
            if (renderCustomMapsSelection(joinableMaps, currentJoinMap, refreshJoinableMaps)) {
                Execute([this](GameWrapper*) {
                    isCurrentMapJoinable = isMapJoinable(currentJoinMap);
                });
            }
        }
        ImGui::Separator();

        ImGui::Indent(5);
        if (isCurrentMapJoinable || !failedToGetMapPackageFileCache) {
            if (ImGui::Button("Join")) {
                Execute([this](GameWrapper*) {
                    JoinGame(pswdBuf);
                });
            }
        }
        else {
            if (ImGui::Button("Copy Map and Restart")) {
                Execute([this](GameWrapper*) {
                    copyMap(absolute(currentJoinMap));
                });
                ImGui::OpenPopup("Map Copied");
            }
        }
        if (ImGui::BeginPopupModal("Map Copied")) {
            ImGui::TextUnformatted("Map copied, please restart your game.");
            if (ImGui::Button("Oke")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    ImGui::EndChild();
}


/*
 *  In game mods
 */

/// <summary>Renders the in game mods tab.</summary>
void RocketPlugin::renderInGameModsTab()
{
    if (ImGui::BeginTabItem("In Game Mods")) {
        if (ImGui::BeginChild("##InGameMods", ImVec2(0, 0), true)) {
            if (ImGui::CollapsingHeader("Game Event Mods")) {
                ImGui::Indent(20);
                if (ImGui::CollapsingHeader("Game Controls")) {
                    if (ImGui::Button("Force Overtime")) {
                        Execute([this](GameWrapper*) {
                            ForceOvertime();
                        });
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Pause Server")) {
                        Execute([this](GameWrapper*) {
                            PauseServer();
                        });
                    }
                    if (ImGui::Button("Restart Match")) {
                        Execute([this](GameWrapper*) {
                            ResetMatch();
                        });
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("End Match")) {
                        Execute([this](GameWrapper*) {
                            EndMatch();
                        });
                    }
                    if (ImGui::Button("Reset Players")) {
                        Execute([this](GameWrapper*) {
                            ResetPlayers();
                        });
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset Balls")) {
                        Execute([this](GameWrapper*) {
                            ResetBalls();
                        });
                    }
                }
                if (ImGui::CollapsingHeader("Match Settings")) {
                    int maxPlayers = GetMaxPlayers();
                    if (ImGui::InputInt("Max Players", &maxPlayers)) {
                        Execute([this, maxPlayers](GameWrapper*) {
                            SetMaxPlayers(maxPlayers);
                        });
                    }
                    int maxTeamSize = GetMaxTeamSize();
                    if (ImGui::InputInt("Max Team Size", &maxTeamSize)) {
                        Execute([this, maxTeamSize](GameWrapper*) {
                            SetMaxTeamSize(maxTeamSize);
                        });
                    }
                    int respawnTime = GetRespawnTime();
                    if (ImGui::InputInt("Respawn Time", &respawnTime)) {
                        Execute([this, respawnTime](GameWrapper*) {
                            SetRespawnTime(respawnTime);
                        });
                    }
                    int blueScore = GetScoreBlue();
                    ImGui::PushItemWidth(150);
                    if (ImGui::InputInt("Blue Score", &blueScore)) {
                        Execute([this, blueScore](GameWrapper*) {
                            SetScoreBlue(blueScore);
                        });
                    }
                    ImGui::SameLine();
                    int orangeScore = GetScoreOrange();
                    if (ImGui::InputInt("Orange Score", &orangeScore)) {
                        Execute([this, orangeScore](GameWrapper*) {
                            SetScoreOrange(orangeScore);
                        });
                    }
                    ImGui::PopItemWidth();
                    int gameTimeRemaining = GetGameTimeRemaining();
                    if (ImGui::DragTime("Time Remaining", &gameTimeRemaining)) {
                        Execute([this, gameTimeRemaining](GameWrapper*) {
                            SetGameTimeRemaining(gameTimeRemaining);
                        });
                    }
                    bool isGoalDelayDisabled = GetIsGoalDelayDisabled();
                    if (ImGui::Checkbox("Disable Goal Delay", &isGoalDelayDisabled)) {
                        Execute([this, isGoalDelayDisabled](GameWrapper*) {
                            SetIsGoalDelayDisabled(isGoalDelayDisabled);
                        });
                    }
                    bool isUnlimitedTime = GetIsUnlimitedTime();
                    if (ImGui::Checkbox("Unlimited Time", &isUnlimitedTime)) {
                        Execute([this, isUnlimitedTime](GameWrapper*) {
                            SetIsUnlimitedTime(isUnlimitedTime);
                        });
                    }
                }
                if (ImGui::CollapsingHeader("Bots")) {
                    int numBots = GetMaxNumBots();
                    if (ImGui::InputInt("Max # bots per team", &numBots)) {
                        if (!gameWrapper->GetGameEventAsServer().IsNull()) {
                            Execute([this, numBots](GameWrapper*) {
                                SetNumBotsPerTeam(numBots);
                            });
                        }
                    }
                    bool isAutoFilledWithBots = GetIsAutoFilledWithBots();
                    if (ImGui::Checkbox("Autofill with bots", &isAutoFilledWithBots)) {
                        Execute([this, isAutoFilledWithBots](GameWrapper*) {
                            SetIsAutoFilledWithBots(isAutoFilledWithBots);
                        });
                    }
                    ImGui::SameLine();
                    bool isUnfairTeams = GetIsUnfairTeams();
                    if (ImGui::Checkbox("Unfair Teams", &isUnfairTeams)) {
                        Execute([this, isUnfairTeams](GameWrapper*) {
                            SetIsUnfairTeams(isUnfairTeams);
                        });
                    }
                    if (ImGui::Button("Freeze Bots")) {
                        Execute([this](GameWrapper*) {
                            FreezeBots();
                        });
                    }
                }
                ImGui::Unindent();
            }
            if (ImGui::CollapsingHeader("Ball Mods")) {
                int numBalls = GetNumBalls();
                if (ImGui::InputInt("# Balls", &numBalls)) {
                    Execute([this, numBalls](GameWrapper*) {
                        SetNumBalls(numBalls);
                    });
                }
                float ballScale = GetBallsScale();
                if (ImGui::SliderFloat("Balls Scale", &ballScale, 0.1f, 10.0f, "%.1fX")) {
                    Execute([this, ballScale](GameWrapper*) {
                        SetBallsScale(ballScale);
                    });
                }
                float maxBallVelocity = GetMaxBallVelocity();
                if (ImGui::DragFloat("Max Ball Velocity", &maxBallVelocity, 1.0f, 0.0f, 0.0f, "%.3f u/s")) {
                    Execute([this, maxBallVelocity](GameWrapper*) {
                        SetMaxBallVelocity(maxBallVelocity);
                    });
                }
            }
            if (ImGui::CollapsingHeader("Player Mods")) {
                std::vector<PriWrapper> players;
                if (IsInGame()) {
                    players = GetPlayers(true);
                }
                ImGui::BeginColumns("Mutators", 4);
                {
                    ImGui::SetColumnWidth(0, 110);
                    ImGui::SetColumnWidth(1, 55);
                    ImGui::SetColumnWidth(2, 55);
                    ImGui::TextUnformatted("Player");
                    ImGui::NextColumn();
                    ImGui::TextUnformatted("Admin");
                    ImGui::NextColumn();
                    ImGui::TextUnformatted("Hidden");
                    ImGui::NextColumn();
                    ImGui::NextColumn();
                    ImGui::Separator();
                    for (PriWrapper& player : players) {
                        const std::string displayName = player.GetPlayerName().ToString();
                        const std::string uniqueName = player.GetUniqueIdWrapper().str();
                        if (player.GetbBot()) {
                            ImGui::Text("(bot) %s", displayName.c_str());
                        }
                        else {
                            ImGui::TextUnformatted(displayName.c_str());
                        }
                        ImGui::NextColumn();
                        bool isAdmin = GetIsAdmin(player);
                        if (ImGui::Checkbox(("##Admin_" + uniqueName).c_str(), &isAdmin)) {
                            Execute([this, player, isAdmin](GameWrapper*) {
                                SetIsAdmin(player, isAdmin);
                            });
                        }
                        ImGui::NextColumn();
                        bool isHidden = GetIsHidden(player);
                        if (ImGui::Checkbox(("##Hidden_" + uniqueName).c_str(), &isHidden)) {
                            Execute([this, player, isHidden](GameWrapper*) {
                                SetIsHidden(player, isHidden);
                            });
                        }
                        ImGui::NextColumn();
                        if (ImGui::Button(("Demolish##_" + uniqueName).c_str())) {
                            Execute([this, player](GameWrapper*) {
                                Demolish(player);
                            });
                        }
                        ImGui::NextColumn();
                        ImGui::Separator();
                    }
                    if (players.empty()) {
                        ImGui::TextUnformatted("No players found");
                    }
                }
                ImGui::EndColumns();
            }
            if (ImGui::CollapsingHeader("Car Physics Mods")) {
                if (!carPhysics.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                    ImGui::TextUnformatted("Car physics mods are active");
                    ImGui::PopStyleColor();
                }

                std::vector<PriWrapper> players;
                std::vector<std::string> playersNames;
                if (IsInGame()) {
                    players = GetPlayers(true);
                    playersNames = GetPlayersNames(players);
                }
                ImGui::TextUnformatted("Modify: ");
                ImGui::SameLine();
                ImGui::Combo("##Players", &selectedPlayer, playersNames, "No players found");
                ImGui::Separator();

                if (selectedPlayer < players.size()) {
                    const PriWrapper player = players[selectedPlayer];
                    CarPhysics playerPhysics = GetPhysics(player);
                    if (ImGui::SliderFloat("Car Scale", &playerPhysics.CarScale, 0.1f, 2.0f, "%.1fX")) {
                        Execute([this, player, newCarScale = playerPhysics.CarScale](GameWrapper*) {
                            SetCarScale(player, newCarScale, true);
                        });
                    }
                    if (ImGui::Checkbox("Car Collision", &playerPhysics.CarHasCollision)) {
                        Execute([this, player, newCarHasCollision = playerPhysics.CarHasCollision](GameWrapper*) {
                            SetbCarCollision(player, newCarHasCollision);
                        });
                    }
                    ImGui::SameLine();
                    if (ImGui::Checkbox("Freeze car", &playerPhysics.CarIsFrozen)) {
                        Execute([this, player, newCarIsFrozen = playerPhysics.CarIsFrozen](GameWrapper*) {
                            SetCarIsFrozen(player, newCarIsFrozen);
                        });
                    }
                    ImGui::Separator();

                    if (ImGui::DragFloat("Torque Rate", &playerPhysics.TorqueRate, 0.1f, 0.0f, 0.0f, "%.3fx10^5 N*m")) {
                        Execute([this, player, newTorqueRate = playerPhysics.TorqueRate](GameWrapper*) {
                            SetTorqueRate(player, newTorqueRate);
                        });
                    }
                    if (ImGui::DragFloat("Max Car Velocity", &playerPhysics.MaxCarVelocity, 1.0f, 0.0f, 0.0f, "%.3f u/s")) {
                        Execute([this, player, newMaxCarVelocity = playerPhysics.MaxCarVelocity](GameWrapper*) {
                            SetMaxCarVelocity(player, newMaxCarVelocity);
                        });
                    }
                    if (ImGui::DragFloat("Ground Sticky Force", &playerPhysics.GroundStickyForce, 1.0f, 0.0f, 0.0f, "%.3f N")) {
                        Execute([this, player, newGroundStickyForce = playerPhysics.GroundStickyForce](GameWrapper*) {
                            SetGroundStickyForce(player, newGroundStickyForce);
                        });
                    }
                    if (ImGui::DragFloat("Wall Sticky Force", &playerPhysics.WallStickyForce, 1.0f, 0.0f, 0.0f, "%.3f N")) {
                        Execute([this, player, newWallStickyForce = playerPhysics.WallStickyForce](GameWrapper*) {
                            SetWallStickyForce(player, newWallStickyForce);
                        });
                    }
                }
            }
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
}


/*
 *  Game modes
 */

/// <summary>Renders the game modes tab.</summary>
void RocketPlugin::renderGameModesTab()
{
    if (ImGui::BeginTabItem("GameModes")) {
        if (ImGui::BeginChild("##GameModesList", ImVec2(200, 0), true)) {
            ImGui::TextUnformatted("available game modes:");
            ImGui::Separator();
            for (size_t i = 0; i < customGameModes.size(); i++) {
                std::shared_ptr<RocketGameMode>& customGameMode = customGameModes[i];
                std::string gameModeName;
                bool gameModeActive = false;
                if (customGameMode == nullptr) {
                    gameModeName = "Error";
                    ImGui::BeginDisabled();
                }
                else {
                    gameModeName = customGameMode->GetGameModeName();
                    gameModeActive = customGameMode->IsActive();
                }
                if (ImGui::SwitchCheckbox(("##" + gameModeName + std::to_string(i)).c_str(), &gameModeActive)) {
                    Execute([this, customGameMode, gameModeActive](GameWrapper*) {
                        customGameMode->Activate(gameModeActive);
                    });
                }
                ImGui::SameLine();
                const float backupButtonTextAlignX = ImGui::GetStyle().ButtonTextAlign.x;
                const float backupFrameRounding = ImGui::GetStyle().FrameRounding;
                ImGui::GetStyle().ButtonTextAlign.x = 0;
                ImGui::GetStyle().FrameRounding = 3;
                if (ImGui::ButtonEx(gameModeName.c_str(), ImVec2(-1, 0), ImGuiButtonFlags_AlignTextBaseLine)) {
                    customGameModeSelected = i;
                }
                ImGui::GetStyle().ButtonTextAlign.x = backupButtonTextAlignX;
                ImGui::GetStyle().FrameRounding = backupFrameRounding;
                if (customGameMode == nullptr) {
                    ImGui::EndDisabled();
                }
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        if (ImGui::BeginChild("##GameModesOptions", ImVec2(0, 0), true)) {
            if (customGameModeSelected < customGameModes.size()) {
                std::shared_ptr<RocketGameMode> customGameMode = customGameModes[customGameModeSelected];
                if (customGameMode != nullptr) {
                    ImGui::TextUnformatted(customGameMode->GetGameModeName().c_str());
                    ImGui::Separator();

                    if (customGameMode->IsActive()) {
                        customGameMode->RenderOptions();
                    }
                    else {
                        ImGui::BeginDisabled();
                        customGameMode->RenderOptions();
                        ImGui::EndDisabled();
                    }
                }
                else {
                    ImGui::TextUnformatted("Failed to load game mode.");
                }
            }
            else {
                ImGui::TextUnformatted("No game modes found.");
            }
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
}
