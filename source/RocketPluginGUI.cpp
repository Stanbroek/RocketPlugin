// RocketPluginGUI.cpp
// GUI for the RocketPlugin plugin.
//
// Author:        Stanbroek
// Version:       0.6.4 24/12/20
// BMSDK version: 95

#include "RocketPlugin.h"

#include "imgui/imgui_internal.h"

#define IM_COL32_ERROR        (ImColor(IM_COL32(204, 0,  0,  255)))
#define IM_COL32_ERROR_BANNER (ImColor(IM_COL32(211, 47, 47, 255)))


/// <summary>ImGui widgets to rendered every tick.</summary>
void RocketPlugin::Render()
{
    IMGUI_CHECKVERSION();
    if (refreshRLConstants) {
        refreshRLConstants = false;
        Execute([this](GameWrapper*) {
            loadRLConstants();
        });
    }

#ifdef DEBUG
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
            ImGui::SearchableCombo("##Maps", currentMap, maps, "No maps found", "type to search");
        }
        else {
            std::filesystem::path currentMapPath = currentMap;
            if (renderCustomMapsSelection(customMapPaths, currentMapPath, refreshCustomMapPaths, enableWorkshopMaps, enableCustomMaps)) {
                currentMap = currentMapPath.string();
            }
        }
        ImGui::TextUnformatted(" Player count:");
        if (ImGui::SliderInt("##PlayerCount", &playerCount, 2, 8, "%d players") && playerCount < 2) {
            playerCount = 6;
        }
        ImGui::TextUnformatted(" Bot Difficulty:");
        ImGui::SliderArray("##BotDifficulty", &botDifficulties.CurrentSelected, botDifficulties.DisplayName);
        // TODO, enable once it works again.
        ImGui::BeginDisabled();
        if (ImGui::CollapsingHeader("Team Settings")) {
            ImGui::TextUnformatted("Team 1");
            ImGui::BeginColumns("Team 1", 2, ImGuiColumnsFlags_NoBorder);
            {
                ImGui::SetColumnWidth(0, 100);
                ImGui::SetCursorPosX(32);
                ImGui::TextUnformatted("Team Name:");
                ImGui::NextColumn();

                ImGui::SetNextItemWidth(100);
                ImGui::InputTextWithHint("##Team1Name", "BLUE", team1Name, sizeof team1Name);
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
                ImGui::InputTextWithHint("##Team2Name", "ORANGE", team2Name, sizeof team2Name);
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
        ImGui::EndDisabled();
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
            static char hostPswdBuf[64] = "";
            strcpy_s(hostPswdBuf, hostPswd.c_str());
            ImGui::TextUnformatted(" Password: (optional)");
            if (ImGui::InputText("##pswd_host", hostPswdBuf, 64, ImGuiInputTextFlags_Password)) {
                hostPswd = std::string(hostPswdBuf);
            }
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
                ImGui::PushStyleColor(ImGuiCol_Border, static_cast<ImVec4>(ImColor(255, 60, 0, 80)));
                if (ImGui::BeginChild("##UPnPWarning", ImVec2(0, 50), true)) {
                    ImGui::Dummy(ImVec2(140.0f, 0.0f));
                    ImGui::SameLine();
                    ImGui::TextUnformatted("WARNING these are advanced settings!");
                    ImGui::TextUnformatted("Using these without knowing what your are doing could put your computer and network at risk.");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                if (ImGui::Button("Search for UPnP devices")) {
                    if (!upnpClient->discoverySearching()) {
                        Execute([this](GameWrapper*) {
                            upnpClient->findDevices();
                        });
                    }
                    else {
                        LOG("Already searching for UPnP devices");
                    }
                }
                ImGui::SameLine();
                if (upnpClient->discoveryFailed()) {
                    ImGui::TextColoredWrapped(IM_COL32_ERROR, upnpClient->getDiscoveryStatus().c_str());
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Make sure your router supports UPnP \nand has it enabled it the settings.");
                    }
                }
                else {
                    ImGui::TextWrapped(upnpClient->getDiscoveryStatus());
                }
                const bool invalidHostPortExternal = !Networking::IsValidPort(hostPortExternal);
                if (upnpClient->discoveryFinished()) {
                    if (ImGui::Button(("Forward port " + (invalidHostPortExternal ? std::to_string(DEFAULT_PORT) : std::to_string(hostPortExternal))).c_str())) {
                        if (!upnpClient->serviceForwardPortActive()) {
                            Execute([=](GameWrapper*) {
                                upnpClient->forwardPort(invalidHostPortExternal ? DEFAULT_PORT : hostPortInternal,
                                    invalidHostPortExternal ? DEFAULT_PORT : hostPortExternal, portLeaseDuration);
                            });
                        }
                        else {
                            LOG("Already forwarding a port");
                        }
                    }
                    if (upnpClient->serviceForwardPortFailed()) {
                        ImGui::SameLine();
                        ImGui::TextColoredWrapped(IM_COL32_ERROR, upnpClient->getForwardPortStatus().c_str());
                    }
                    else if (upnpClient->serviceForwardPortFinished()) {
                        ImGui::SameLine();
                        ImGui::TextWrapped(upnpClient->getForwardPortStatus());
                    }
                }
                ImGui::TextUnformatted("External Address:");
                const float itemWidth = std::max(0.f, ImGui::GetCurrentWindowRead()->DC.ItemWidth - ImGui::CalcTextSize(":").x);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
                ImGui::PushItemWidth(itemWidth / 3 * 2);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                ImGui::InputText("##ExternalIPAddress", upnpClient->getExternalIPAddressBuffer(), 64, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
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
                if (upnpClient->discoveryFinished()) {
                    ImGui::TextUnformatted("Open ports:");
                    ImGui::SameLine();
                    if (upnpClient->serviceClosePortFailed()) {
                        ImGui::TextColoredWrapped(IM_COL32_ERROR, upnpClient->getClosePortStatus().c_str());
                    }
                    else if (upnpClient->serviceClosePortFinished()) {
                        ImGui::TextWrapped(upnpClient->getClosePortStatus());
                    }
                    const std::vector<unsigned short>& openPorts = upnpClient->getOpenPorts();
                    if (openPorts.empty()) {
                        ImGui::TextUnformatted("\tNo open ports found.");
                    }
                    else {
                        for (const unsigned short openPort : openPorts) {
                            if (ImGui::Button(std::to_string(openPort).c_str())) {
                                if (!upnpClient->serviceClosePortActive()) {
                                    Execute([this, openPort = openPort](GameWrapper*) {
                                        upnpClient->closePort(openPort);
                                    });
                                }
                                else {
                                    LOG("Already closing a port");
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
                ImGui::PushStyleColor(ImGuiCol_Border, static_cast<ImVec4>(ImColor(255, 60, 0, 80)));
                if (ImGui::BeginChild("##P2PWarning", ImVec2(0, 50), true)) {
                    ImGui::Dummy(ImVec2(140.0f, 0.0f));
                    ImGui::SameLine();
                    ImGui::TextUnformatted("WARNING these are advanced settings!");
                    ImGui::TextUnformatted("Using these without knowing what your are doing could put your computer and network at risk.");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                if (ImGui::Button("get NAT type")) {
                    if (p2pHost->getNATType() != P2PHost::NATType::NAT_SEARCHING) {
                        Execute([this](GameWrapper*) {
                            p2pHost->findNATType(hostPortInternal);
                        });
                    }
                    else {
                        LOG("Already getting NAT type.");
                    }
                }
                ImGui::SameLine();
                ImGui::TextWrapped(p2pHost->getNATDesc());
                const bool invalidHostPortExternal = !Networking::IsValidPort(hostPortExternal);
                if (p2pHost->getNATType() == P2PHost::NATType::NAT_FULL_CONE || p2pHost->getNATType() == P2PHost::NATType::NAT_RESTRICTED_PORT) {
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
                switch (p2pHost->getNATType()) {
                    case P2PHost::NATType::NAT_FULL_CONE:
                        if (ImGui::Button(("Punch through port " + std::to_string(invalidHostPortExternal ? DEFAULT_PORT : hostPortExternal)).c_str())) {
                            Execute([=](GameWrapper*) {
                                p2pHost->punchPort(NAT_PUNCH_ADDR, invalidHostPortExternal ? DEFAULT_PORT : hostPortExternal);
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
                                if (ImGui::InputText(("##P2PClientIP" + std::to_string(i)).c_str(), &connections[i].IP[0], 64)) {
                                    connections[i].InvalidIP = !Networking::IsValidIPv4(connections[i].IP);
                                }
                                ImGui::EndErrorBorder();
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip("Invalid ipv4");
                                }
                            }
                            else {
                                if (ImGui::InputText(("##P2PClientIP" + std::to_string(i)).c_str(), &connections[i].IP[0], 64)) {
                                    connections[i].InvalidIP = !Networking::IsValidIPv4(connections[i].IP);
                                }
                            }
                            ImGui::SameLine();
                            if (ImGui::Button(("Start Connection##P2PClientConn" + std::to_string(i)).c_str())) {
                                p2pHost->punchPort(connections[i].IP, hostPortExternal);
                                Execute([this, addr = connections[i].IP](GameWrapper*) {
                                    p2pHost->punchPort(addr, hostPortExternal);
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
        }
        ImGui::Separator();

        if (ImGui::Button("Host")) {
            Execute([this](GameWrapper*) {
                hostGame();
            });
        }
    }
    ImGui::EndChild();
}


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
        static char ipBuf[64] = "";
        strcpy_s(ipBuf, joinIP->c_str());
        static Networking::HostStatus hostOnline = Networking::HostStatus::HOST_UNKNOWN;
        static Networking::DestAddrType addressType = Networking::GetDestAddrType(ipBuf);
        BeginHostStatusBorder(addressType, hostOnline);
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
        if (hostOnline == Networking::HostStatus::HOST_BUSY) {
            flags |= ImGuiInputTextFlags_ReadOnly;
        }
        if (ImGui::InputText("##ip_join", ipBuf, 64, flags)) {
            *joinIP = std::string(ipBuf);
            cvarManager->getCvar("mp_ip").setValue(*joinIP);
            addressType = Networking::GetDestAddrType(ipBuf);
            if (hostOnline != Networking::HostStatus::HOST_BUSY) {
                hostOnline = Networking::HostStatus::HOST_UNKNOWN;
            }
        }
        ImGui::EndBorder();
        if (hostOnline == Networking::HostStatus::HOST_UNKNOWN && addressType != Networking::DestAddrType::UNKNOWN_ADDR) {
            // Only ping when the user is finished typing.
            if (ImGui::IsItemActiveLastFrame() && !ImGui::IsItemActive()) {
                Networking::PingHost(ipBuf, static_cast<unsigned short>(*joinPort), &hostOnline, true);
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
        if (isCurrentMapJoinable) {
            if (ImGui::Button("Join")) {
                Execute([this, pswd = pswdBuf](GameWrapper*) {
                    joinGame(pswd);
                });
            }
        }
        else {
            if (ImGui::Button("Copy Map and Restart")) {
                Execute([this](GameWrapper*) {
                    copyMap(absolute(currentJoinMap));
                });
            }
        }
    }
    ImGui::EndChild();
}


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
                            forceOvertime();
                        });
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Pause Server")) {
                        Execute([this](GameWrapper*) {
                            pauseServer();
                        });
                    }
                    if (ImGui::Button("Restart Match")) {
                        Execute([this](GameWrapper*) {
                            resetMatch();
                        });
                    }
                    if (ImGui::Button("Reset Players")) {
                        Execute([this](GameWrapper*) {
                            resetPlayers();
                        });
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset Balls")) {
                        Execute([this](GameWrapper*) {
                            resetBalls();
                        });
                    }
                }
                if (ImGui::CollapsingHeader("Match Settings")) {
                    int maxPlayers = getMaxPlayers();
                    if (ImGui::InputInt("Max Players", &maxPlayers)) {
                        Execute([this, newMaxPlayers = maxPlayers](GameWrapper*) {
                            setMaxPlayers(newMaxPlayers);
                        });
                    }
                    int maxTeamSize = getMaxTeamSize();
                    if (ImGui::InputInt("Max Team Size", &maxTeamSize)) {
                        Execute([this, newMaxTeamSize = maxTeamSize](GameWrapper*) {
                            setMaxTeamSize(newMaxTeamSize);
                        });
                    }
                    int respawnTime = getRespawnTime();
                    if (ImGui::InputInt("Respawn Time", &respawnTime)) {
                        Execute([this, newRespawnTime = respawnTime](GameWrapper*) {
                            setRespawnTime(newRespawnTime);
                        });
                    }
                    int blueScore = getScoreBlue();
                    ImGui::PushItemWidth(150);
                    if (ImGui::InputInt("Blue Score", &blueScore)) {
                        Execute([this, newBlueScore = blueScore](GameWrapper*) {
                            setScoreBlue(newBlueScore);
                        });
                    }
                    ImGui::SameLine();
                    int orangeScore = getScoreOrange();
                    if (ImGui::InputInt("Orange Score", &orangeScore)) {
                        Execute([this, newOrangeScore = orangeScore](GameWrapper*) {
                            setScoreOrange(newOrangeScore);
                        });
                    }
                    ImGui::PopItemWidth();
                    int gameTimeRemaining = getGameTimeRemaining();
                    if (ImGui::DragTime("Time Remaining", &gameTimeRemaining)) {
                        Execute([this, newGameTimeRemaining = gameTimeRemaining](GameWrapper*) {
                            setGameTimeRemaining(newGameTimeRemaining);
                        });
                    }
                    bool isGoalDelayDisabled = getIsGoalDelayDisabled();
                    if (ImGui::Checkbox("Disable Goal Delay", &isGoalDelayDisabled)) {
                        Execute([this, isGoalDelayDisabled = isGoalDelayDisabled](GameWrapper*) {
                            setIsGoalDelayDisabled(isGoalDelayDisabled);
                        });
                    }
                    bool isUnlimitedTime = getIsUnlimitedTime();
                    if (ImGui::Checkbox("Unlimited Time", &isUnlimitedTime)) {
                        Execute([this, isUnlimitedTime = isUnlimitedTime](GameWrapper*) {
                            setIsUnlimitedTime(isUnlimitedTime);
                        });
                    }
                }
                if (ImGui::CollapsingHeader("Bots")) {
                    int numBots = getMaxNumBots();
                    if (ImGui::InputInt("Max # bots per team", &numBots)) {
                        if (!gameWrapper->GetGameEventAsServer().IsNull()) {
                            Execute([this, newNumBots = numBots](GameWrapper*) {
                                prepareBots(newNumBots);
                            });
                        }
                    }
                    bool isAutoFilledWithBots = getIsAutoFilledWithBots();
                    if (ImGui::Checkbox("Autofill with bots", &isAutoFilledWithBots)) {
                        Execute([this, isAutoFilledWithBots = isAutoFilledWithBots](GameWrapper*) {
                            setIsAutoFilledWithBots(isAutoFilledWithBots);
                        });
                    }
                    ImGui::SameLine();
                    bool isUnfairTeams = getIsUnfairTeams();
                    if (ImGui::Checkbox("Unfair Teams", &isUnfairTeams)) {
                        Execute([this, isUnfairTeams = isUnfairTeams](GameWrapper*) {
                            setIsUnfairTeams(isUnfairTeams);
                        });
                    }
                    if (ImGui::Button("Freeze Bots")) {
                        Execute([this](GameWrapper*) {
                            freezeBots();
                        });
                    }
                }
                ImGui::Unindent();
            }
            if (ImGui::CollapsingHeader("Ball Mods")) {
                int numBalls = getNumBalls();
                if (ImGui::InputInt("# Balls", &numBalls)) {
                    Execute([this, newNumBalls = numBalls](GameWrapper*) {
                        setNumBalls(newNumBalls);
                    });
                }
                float ballScale = getBallsScale();
                if (ImGui::SliderFloat("Balls Scale", &ballScale, 0.1f, 10.0f, "%.1fX")) {
                    Execute([this, newBallScale = ballScale](GameWrapper*) {
                        setBallsScale(newBallScale);
                    });
                }
            }
            if (ImGui::CollapsingHeader("Player Mods")) {
                std::vector<PriWrapper> players = getPlayers(true);
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
                        ImGui::TextUnformatted(displayName.c_str());
                        ImGui::NextColumn();
                        bool isAdmin = getIsAdmin(player);
                        if (ImGui::Checkbox(("##Admin_" + uniqueName).c_str(), &isAdmin)) {
                            Execute([this, player = player, isAdmin = isAdmin](GameWrapper*) {
                                setIsAdmin(player, isAdmin);
                            });
                        }
                        ImGui::NextColumn();
                        bool isHidden = getIsHidden(player);
                        if (ImGui::Checkbox(("##Hidden_" + uniqueName).c_str(), &isHidden)) {
                            Execute([this, player = player, isHidden = isHidden](GameWrapper*) {
                                setIsHidden(player, isHidden);
                            });
                        }
                        ImGui::NextColumn();
                        if (ImGui::Button(("Demolish##_" + uniqueName).c_str())) {
                            Execute([this, player = player](GameWrapper*) {
                                demolish(player);
                            });
                        }
                        ImGui::NextColumn();
                        ImGui::Separator();
                    }
                }
                ImGui::EndColumns();
            }
            if (ImGui::CollapsingHeader("Car Physics Mods")) {
                const std::vector<PriWrapper> players = getPlayers(true);
                const std::vector<std::string> playersNames = getPlayersNames(players);
                ImGui::TextUnformatted("Modify: ");
                ImGui::SameLine();
                ImGui::Combo("##Players", &selectedPlayer, playersNames, "No players found");
                ImGui::Separator();

                if (selectedPlayer < players.size()) {
                    const PriWrapper player = players[selectedPlayer];
                    CarPhysics playerPhysics = getPhysics(player);
                    if (ImGui::SliderFloat("Car Scale", &playerPhysics.CarScale, 0.1f, 2.0f, "%.1fX")) {
                        Execute([this, player = player, newCarScale = playerPhysics.CarScale](GameWrapper*) {
                            setCarScale(player, newCarScale);
                        });
                    }
                    if (ImGui::Checkbox("Car Collision", &playerPhysics.CarHasCollision)) {
                        Execute([this, player = player, carHasCollision = playerPhysics.CarHasCollision](GameWrapper*) {
                            setbCarCollision(player, carHasCollision);
                        });
                    }
                    ImGui::SameLine();
                    if (ImGui::Checkbox("Freeze car", &playerPhysics.CarIsFrozen)) {
                        Execute([this, player = player, carIsFrozen = playerPhysics.CarIsFrozen](GameWrapper*) {
                            setCarIsFrozen(player, carIsFrozen);
                        });
                    }
                    ImGui::Separator();

                    if (ImGui::DragFloat("Torque Rate", &playerPhysics.TorqueRate, 0.1f, 0.0f, 0.0f, "%.3fx10^5 N*m")) {
                        Execute([this, player = player, torqueRate = playerPhysics.TorqueRate](GameWrapper*) {
                            setTorqueRate(player, torqueRate);
                        });
                    }
                    if (ImGui::DragFloat("Max Car Velocity", &playerPhysics.MaxCarVelocity, 1.0f, 0.0f, 0.0f, "%.3f u/s")) {
                        Execute([this, player = player, maxCarVelocity = playerPhysics.MaxCarVelocity](GameWrapper*) {
                            setMaxCarVelocity(player, maxCarVelocity);
                        });
                    }
                    if (ImGui::DragFloat("Ground Sticky Force", &playerPhysics.GroundStickyForce, 1.0f, 0.0f, 0.0f, "%.3f N")) {
                        Execute([this, player = player, groundStickyForce = playerPhysics.GroundStickyForce](GameWrapper*) {
                            setGroundStickyForce(player, groundStickyForce);
                        });
                    }
                    if (ImGui::DragFloat("Wall Sticky Force", &playerPhysics.WallStickyForce, 1.0f, 0.0f, 0.0f, "%.3f N")) {
                        Execute([this, player = player, wallStickyForce = playerPhysics.WallStickyForce](GameWrapper*) {
                            setWallStickyForce(player, wallStickyForce);
                        });
                    }
                }
            }
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
}


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
                    Execute([this, customGameMode = customGameMode, gameModeActive = gameModeActive](GameWrapper*) {
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
            for (const std::filesystem::path& workshopMap : getWorkshopMaps(*workshopMapDirPath)) {
                if (auto it = subscribedWorkshopMaps.find(
                        std::strtoull(workshopMap.parent_path().stem().string().c_str(), nullptr, 10));
                    it != subscribedWorkshopMaps.end() && !it->second.Title.empty()) {
                    customMaps.insert({workshopMap, it->second.Title});
                }
                else {
                    customMaps.insert({workshopMap, workshopMap.stem().u8string()});
                }
            }
        }
        if (includeCustomMaps) {
            for (const std::filesystem::path& customMap : GetFilesFromDir(*customMapDirPath, 2, ".upk", ".udk")) {
                if (auto it = subscribedWorkshopMaps.find(
                        std::strtoull(customMap.parent_path().stem().string().c_str(), nullptr, 10));
                    it != subscribedWorkshopMaps.end() && !it->second.Title.empty()) {
                    customMaps.insert({customMap, it->second.Title});
                }
                else {
                    customMaps.insert({customMap, customMap.stem().u8string()});
                }
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
