// RocketPluginGUI.cpp
// GUI for the RocketPlugin plugin.
//
// Author:       Stanbroek
// Version:      0.6.3 15/7/20
// BMSDKversion: 95

#include "RocketPlugin.h"

#include "imgui/imgui_internal.h"

#define IM_COL32_ERROR_RED (ImColor)IM_COL32(204,0,0,255)


/// <summary>ImGui widgets to rendered every tick.</summary>
void RocketPlugin::Render()
{
    IMGUI_CHECKVERSION();
    //ImGui::ShowDemoWindow();
    //ImGui::ShowMetricsWindow();

    ImGui::SetNextWindowSizeConstraints(ImVec2(800, 600), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin(menuTitle.c_str(), &isWindowOpen);
    {
        if (ImGui::BeginTabBar("#TabBar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_NoTooltip)) {
            if (ImGui::BeginTabItem("Multiplayer")) {
                if (ImGui::BeginChild("#HostGame", ImVec2(-ImGui::GetFrameWidthWithSpacing() * 6, -ImGui::GetFrameHeightWithSpacing() + 23), true)) {
                    ImGui::Indent(5);
                    ImGui::Spacing();

                    ImGui::Text("Host a local game");
                    ImGui::Separator();

                    ImGui::Text(" Game Mode");
                    if (ImGui::Combo("##GameMode", &currentGameMode, gameModes, "No maps found")) {
                        // Check for godball.
                        if (currentGameMode == 4) {
                            gameWrapper->Execute([this](GameWrapper*) {
                                addGodBall();
                            });
                        }
                    }
                    ImGui::Text(" Arena");
                    ImGui::SameLine();
                    if (ImGui::Checkbox("Enable workshop maps", &enableWorkshopMaps)) {
                        currentMap = 0;
                        otherMapPaths.clear();
                    }
                    ImGui::SameLine();
                    if (ImGui::Checkbox("Enable custom maps", &enableCustomMaps)) {
                        currentMap = 0;
                        otherMapPaths.clear();
                    }
                    if (!enableWorkshopMaps && !enableCustomMaps) {
                        ImGui::SearchableCombo("##Maps", &currentMap, maps, "No maps found", "type to search");
                    }
                    else {
                        if (currentMap >= (int)otherMapPaths.size()) {
                            currentMap = 0;
                            otherMapPaths.clear();
                            if (enableWorkshopMaps) {
                                std::vector<std::filesystem::path> workshopMaps = getWorkshopMaps(*workshopMapDirPath);
                                otherMapPaths.insert(std::end(otherMapPaths), std::begin(workshopMaps), std::end(workshopMaps));
                            }
                            if (enableCustomMaps) {
                                std::vector<std::filesystem::path> customMaps = getFilesFromDir(*customMapDirPath, 2, ".upk", ".udk");
                                otherMapPaths.insert(std::end(otherMapPaths), std::begin(customMaps), std::end(customMaps));
                            }
                        }
                        const int input_size = 32;
                        char input_buffer[input_size] = "";
                        std::string preview_value = "No maps found";
                        if ((int)otherMapPaths.size() > currentMap) {
                            preview_value = otherMapPaths[currentMap].filename().string();
                        }
                        bool buffer_changed = false;
                        if (ImGui::BeginSearchableCombo("##Maps", preview_value.c_str(), input_buffer, input_size, "type to search", &buffer_changed)) {
                            // Only refresh maps on user interaction.
                            if (buffer_changed) {
                                otherMapPaths.clear();
                                if (enableWorkshopMaps) {
                                    std::vector<std::filesystem::path> workshopMaps = getWorkshopMaps(*workshopMapDirPath);
                                    otherMapPaths.insert(std::end(otherMapPaths), std::begin(workshopMaps), std::end(workshopMaps));
                                }
                                if (enableCustomMaps) {
                                    std::vector<std::filesystem::path> customMaps = getFilesFromDir(*customMapDirPath, 2, ".upk", ".udk");
                                    otherMapPaths.insert(std::end(otherMapPaths), std::begin(customMaps), std::end(customMaps));
                                }
                            }

                            int matched_items = 0;
                            std::filesystem::path last_path;
                            for (int i = 0; i < (int)otherMapPaths.size(); i++) {
                                char buffer[input_size] = "";
                                ImStrncpy(buffer, input_buffer, input_size);
                                std::string item_text = otherMapPaths[i].filename().string();
                                // Allow filtering on file and workshop name.
                                std::string folder_name = otherMapPaths[i].parent_path().filename().string();
                                if (enableWorkshopMaps) {
                                    uint64_t workshopMapId = std::atoll(folder_name.c_str());
                                    if (subscribedWorkshopMaps.find(workshopMapId) != subscribedWorkshopMaps.end()) {
                                        WorkshopMap workshopMap = subscribedWorkshopMaps[workshopMapId];
                                        if (!workshopMap.title.empty()) {
                                            folder_name = workshopMap.title;
                                        }
                                    }
                                }

                                if (toLower(item_text).find(toLower(buffer), 0) == std::string::npos && toLower(folder_name).find(toLower(buffer), 0) == std::string::npos) {
                                    continue;
                                }

                                ImGui::PushID((void*)(intptr_t)i);
                                const bool item_selected = (i == currentMap);
                                if (otherMapPaths[i].parent_path() != last_path) {
                                    last_path = otherMapPaths[i].parent_path();
                                    ImGui::Selectable((">" + folder_name).c_str(), false, ImGuiSelectableFlags_Disabled);
                                }
                                if (ImGui::Selectable(item_text.c_str(), item_selected)) {
                                    currentMap = i;
                                }
                                matched_items++;
                                if (item_selected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                                ImGui::PopID();
                            }
                            if (matched_items == 0) {
                                ImGui::Selectable("No maps found", false, ImGuiSelectableFlags_Disabled);
                            }

                            ImGui::EndCombo();
                        }
                    }

                    ImGui::Text(" Player count");
                    if (ImGui::SliderInt("##PlayerCount", &playerCount, 2, 8, "%d players") && playerCount < 2) {
                        playerCount = 6;
                    }
                    ImGui::Text(" Bot Difficulty");
                    ImGui::SameLine();
                    ImGui::Checkbox("Autofill with bots", &enableBots);
                    if (!enableBots) {
                        ImGui::BeginDisabled();
                    }
                    ImGui::SliderArray("##BotDifficulty", &botDifficulty, botDifficulties);
                    if (!enableBots) {
                        ImGui::EndDisabled();
                    }
                    ImGui::Checkbox("Enable freeplay", &enableFreeplay);
                    if (ImGui::CollapsingHeader("Team Settings")) {
                        ImGui::Text("Team 1");
                        ImGui::BeginColumns("Team 1", 2, ImGuiColumnsFlags_NoBorder);
                        {
                            ImGui::SetColumnWidth(0, 100);
                            ImGui::SetCursorPosX(32);
                            ImGui::Text("Team Name:");
                            ImGui::NextColumn();

                            ImGui::SetNextItemWidth(100);
                            ImGui::InputTextWithHint("##Team1Name", "BLUE", team1Name, sizeof team1Name);
                            ImGui::NextColumn();

                            ImGui::SetCursorPosX(20);
                            ImGui::Text("Primairy Color:");
                            ImGui::NextColumn();

                            ImGui::RLColorPicker("##Team1PrimairyColor", &team1PrimCol, teamColors, 15, ImVec4(0.098f, 0.451f, 1.000f, 1.000f), ImVec2(100, 0));
                            ImGui::NextColumn();

                            ImGui::SetCursorPosX(28);
                            ImGui::Text("Accent Color:");
                            ImGui::NextColumn();

                            ImGui::RLColorPicker("##Team1AccentColor", &team1AccCol, teamColors, 15, ImVec4(0.522f, 0.808f, 0.902f, 1.000f), ImVec2(100, 0));
                        }
                        ImGui::EndColumns();
                        ImGui::Text("Team 2");
                        ImGui::BeginColumns("Team 2", 2, ImGuiColumnsFlags_NoBorder);
                        {
                            ImGui::SetColumnWidth(0, 100);
                            ImGui::SetCursorPosX(32);
                            ImGui::Text("Team Name:");
                            ImGui::NextColumn();

                            ImGui::SetNextItemWidth(100);
                            ImGui::InputTextWithHint("##Team2Name", "ORANGE", team2Name, sizeof team2Name);
                            ImGui::NextColumn();

                            ImGui::SetCursorPosX(20);
                            ImGui::Text("Primairy Color:");
                            ImGui::NextColumn();

                            ImGui::RLColorPicker("##Team2PrimairyColor", &team2PrimCol, teamColors, 15, ImVec4(0.765f, 0.392f, 0.098f, 1.000f), ImVec2(100, 0));
                            ImGui::NextColumn();

                            ImGui::SetCursorPosX(28);
                            ImGui::Text("Accent Color:");
                            ImGui::NextColumn();

                            ImGui::RLColorPicker("##Team2AccentColor", &team2AccCol, teamColors, 15, ImVec4(0.902f, 0.235f, 0.098f, 1.000f), ImVec2(100, 0));
                        }
                        ImGui::EndColumns();
                        ImGui::Checkbox("Club Match", &clubMatch);
                    }
                    if (ImGui::CollapsingHeader("Mutators Settings")) {
                        if (ImGui::BeginChild("#MutatorsSettings", ImVec2(-ImGui::GetFrameWidthWithSpacing() * 4, mutators.size() * 23.f), false, ImGuiWindowFlags_NoScrollbar)) {
                            ImGui::BeginColumns("Mutators", 2, ImGuiColumnsFlags_NoBorder);
                            {
                                ImGui::SetColumnWidth(0, 125);
                                ImVec2 offset;
                                for (Mutator& mutator : mutators) {
                                    std::string displayName = std::string(mutator.name) + ":";
                                    offset = ImGui::CalcTextSize(displayName.c_str());
                                    ImGui::SetCursorPosX(125 - offset.x - 10);
                                    ImGui::Text(displayName.c_str());
                                    ImGui::NextColumn();

                                    std::string label = "##" + mutator.name;
                                    ImGui::PushItemWidth(ImGui::GetWindowWidth() / 3 * 2);
                                    ImGui::SliderArray(label.c_str(), &mutator.current_selected, mutator.display);
                                    ImGui::NextColumn();
                                }
                            }
                            ImGui::EndColumns();
                        }
                        ImGui::EndChild();
                        ImGui::SameLine();
                        if (ImGui::BeginChild("#MutatorsPresets", ImVec2(0.f, mutators.size() * 23.f))) {
                            ImGui::Text(" Presets:");
                            ImGui::Separator();

                            static std::vector<std::filesystem::path> presetPaths = getFilesFromDir(*presetDirPath, 1, ".cfg");
                            if (ImGui::Button("Default", ImVec2(-5, 0))) {
                                resetMutators();
                            }
                            for (std::filesystem::path presetPath : presetPaths) {
                                std::string filename = presetPath.filename().string();
                                if (ImGui::Button(filename.substr(0, filename.size() - 4).c_str(), ImVec2(-5, 0))) {
                                    loadPreset(presetPath.string());
                                }
                            }
                            if (presetPaths.size() == 0) {
                                ImGui::PushItemWidth(-5);
                                ImGui::Text("No presets found.");
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
                                    presetPaths = getFilesFromDir(*presetDirPath, 1, ".cfg");
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
                        strcpy_s(hostPswdBuf, password->c_str());
                        ImGui::Text(" Password: (optional)");
                        if (ImGui::InputText("##pswd_host", hostPswdBuf, 64, ImGuiInputTextFlags_Password)) {
                            *password = std::string(hostPswdBuf);
                        }
                        ImGui::Text(" Host port: (default is 7777)");
                        static int hostPortBuf = *port;
                        static bool invalidHostPort = hostPortBuf < 1 || hostPortBuf > 65535;
                        if (invalidHostPort) {
                            ImGui::BeginErrorBorder();
                            if (ImGui::InputInt("##post_host", &hostPortBuf, 0)) {
                                invalidHostPort = hostPortBuf < 1 || hostPortBuf > 65535;
                            }
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("Invalid port");
                            }
                            ImGui::EndErrorBorder();
                        }
                        else {
                            if (ImGui::InputInt("##post_host", &hostPortBuf, 0)) {
                                invalidHostPort = hostPortBuf < 1 || hostPortBuf > 65535;
                            }
                        }
                        ImGui::Text(" Network options:");
                        ImGui::Indent(4);
                        bool upnpFailed = (upnpClient == nullptr);
                        if (upnpFailed) {
                            ImGui::BeginDisabled();
                        }
                        if (ImGui::CollapsingHeader("UPnP Settings")) {
                            ImGui::Indent(10);
                            ImGui::PushStyleColor(ImGuiCol_Border, (ImVec4)ImColor(255, 60, 0, 80));
                            if (ImGui::BeginChild("##UPnPWarning", ImVec2(0, 50), true)) {
                                ImGui::Dummy(ImVec2(140.0f, 0.0f));
                                ImGui::SameLine();
                                ImGui::Text("WARNING these are advanced settings!");
                                ImGui::Text("Using these without knowing what your are doing could put your computer and network at risk.");
                            }
                            ImGui::EndChild();
                            ImGui::PopStyleColor();
                            if (ImGui::Button("Search for UPnP devices")) {
                                if (!upnpClient->discoverySearching()) {
                                    gameWrapper->Execute([this, upnpClient = upnpClient](GameWrapper*) {
                                        upnpClient->findDevices();
                                    });
                                }
                                else {
                                    gameWrapper->Execute([this](GameWrapper*) {
                                        cvarManager->log("Already searching for UPnP devices");
                                    });
                                }
                            }
                            ImGui::SameLine();
                            if (upnpClient->discoveryFailed()) {
                                ImGui::TextColoredWrapped(IM_COL32_ERROR_RED, upnpClient->getDiscoveryStatus().c_str());
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip("Make sure your router supports UPnP \nand has it enabled it the settings.");
                                }
                            }
                            else {
                                ImGui::TextWrapped(upnpClient->getDiscoveryStatus().c_str());
                            }
                            if (upnpClient->discoveryFinished()) {
                                if (ImGui::Button(("Forward port " + (invalidHostPort ? "7777" : std::to_string(hostPortBuf))).c_str())) {
                                    if (!upnpClient->serviceAddPortActive()) {
                                        gameWrapper->Execute([this, upnpClient = upnpClient](GameWrapper*) {
                                            upnpClient->forwardPort((invalidHostPort ? 7777 : (unsigned short)hostPortBuf), portLeaseDuration);
                                        });
                                    }
                                    else {
                                        gameWrapper->Execute([this](GameWrapper*) {
                                            cvarManager->log("Already forwarding a port");
                                        });
                                    }
                                }
                                ImGui::SameLine();
                                if (upnpClient->serviceAddPortFailed()) {
                                    ImGui::TextColoredWrapped(IM_COL32_ERROR_RED, upnpClient->getforwardPortStatus().c_str());
                                }
                                else {
                                    ImGui::TextWrapped(upnpClient->getforwardPortStatus().c_str());
                                }
                            }
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                            ImGui::InputText("##ExternalIPAdress", upnpClient->externalIPAddress, 64, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
                            ImGui::PopStyleVar();
                            ImGui::SameLine();
                            ImGui::Text("External IP Adress");
                            ImGui::Text("Duration to open your port for: (0 means indefinite)");
                            ImGui::DragTime(" hour:minutes:seconds##portLeaseDuration", &portLeaseDuration, 60, 0, 604800);
                            if (upnpClient->discoveryFinished()) {
                                ImGui::Text("Open ports:");
                                ImGui::SameLine();
                                if (upnpClient->serviceDeletePortFailed()) {
                                    ImGui::TextColoredWrapped(IM_COL32_ERROR_RED, upnpClient->getclosePortStatus().c_str());
                                }
                                else {
                                    ImGui::TextWrapped(upnpClient->getclosePortStatus().c_str());
                                }
                                if (upnpClient->openPorts.empty()) {
                                    ImGui::Text("\tNo open ports found.");
                                }
                                else {
                                    for (unsigned short openPort : upnpClient->openPorts) {
                                        if (ImGui::Button(std::to_string(openPort).c_str())) {
                                            if (!upnpClient->serviceDeletePortActive()) {
                                                gameWrapper->Execute([this, openPort = openPort](GameWrapper*) {
                                                    upnpClient->closePort(openPort);
                                                });
                                            }
                                            else {
                                                gameWrapper->Execute([this](GameWrapper*) {
                                                    cvarManager->log("Already removing a port");
                                                });
                                            }
                                        }
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip(("Close port " + std::to_string(openPort) + ".").c_str());
                                        }
                                        ImGui::SameLine();
                                    }
                                    ImGui::NewLine();
                                }
                            }
                            ImGui::Dummy(ImVec2(0.0f, 5.0f));
                            ImGui::Text("Special thanks to Martinn for assisting with UPnP.");
                            ImGui::Indent(-10);
                        }
                        if (upnpFailed) {
                            ImGui::EndDisabled();
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("Failed to create networking thread.");
                        }
                        bool p2pFailed = (p2pHost == nullptr);
                        if (p2pFailed)
                            ImGui::BeginDisabled();
                        if (ImGui::CollapsingHeader("P2P Settings")) {
                            ImGui::Indent(10);
                            ImGui::PushStyleColor(ImGuiCol_Border, (ImVec4)ImColor(255, 60, 0, 80));
                            if (ImGui::BeginChild("##P2PWarning", ImVec2(0, 50), true)) {
                                ImGui::Dummy(ImVec2(140.0f, 0.0f));
                                ImGui::SameLine();
                                ImGui::Text("WARNING these are advanced settings!");
                                ImGui::Text("Using these without knowing what your are doing could put your computer and network at risk.");
                            }
                            ImGui::EndChild();
                            ImGui::PopStyleColor();
                            if (ImGui::Button("get NAT type")) {
                                p2pHost->findNATType(7777);
                            }
                            ImGui::SameLine();
                            ImGui::TextWrapped(p2pHost->getNATDesc().c_str());
                            switch (p2pHost->getNATType())
                            {
                            case P2PHost::NATType::NAT_FULL_CONE:
                                if (ImGui::Button(("Punch through port " + std::to_string(*port)).c_str()))
                                    p2pHost->punchPort("3.3.3.3", (unsigned short)*port);
                                break;
                            case P2PHost::NATType::NAT_RESTRIC_PORT:
                                if (connections.size() > 1 && connections[connections.size() - 1].IP[0] == '\0' && connections[connections.size() - 2].IP[0] == '\0')
                                    connections.pop_back();
                                if (connections.empty() || connections.back().IP[0] != '\0') {
                                    connections.push_back({ { '\0' }, true });
                                }
                                for (auto i = 0u; i < connections.size(); i++) {
                                    if (connections[i].invalidIP) {
                                        ImGui::BeginErrorBorder();
                                        if (ImGui::InputText(("##P2PClientIP" + std::to_string(i)).c_str(), &connections[i].IP[0], 64))
                                            connections[i].invalidIP = !Networking::isValidIPv4(connections[i].IP);
                                        if (ImGui::IsItemHovered())
                                            ImGui::SetTooltip("Invalid ipv4");
                                        ImGui::EndErrorBorder();
                                    }
                                    else {
                                        if (ImGui::InputText(("##P2PClientIP" + std::to_string(i)).c_str(), &connections[i].IP[0], 64))
                                            connections[i].invalidIP = !Networking::isValidIPv4(connections[i].IP);
                                    }
                                    ImGui::SameLine();
                                    if (ImGui::Button(("Start Connection##P2PClientConn" + std::to_string(i)).c_str()))
                                        p2pHost->punchPort(connections[i].IP, (unsigned short)*port);
                                }
                                break;
                            default:
                                break;
                            }
                            ImGui::Indent(-10);
                        }
                        if (p2pFailed) {
                            ImGui::EndDisabled();
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("Failed to create networking thread.");
                        }
                        ImGui::Indent(-4);
                        ImGui::Checkbox("Host game with party", &hostWithParty);
                    }
                    ImGui::Separator();

                    if (ImGui::Button("Host")) {
                        gameWrapper->Execute([this](GameWrapper*) {
                            hostGame();
                        });
                    }
                }
                ImGui::EndChild();
                ImGui::SameLine();
                if (ImGui::BeginChild("#JoinGame", ImVec2(0, 0), true)) {
                    ImGui::PushItemWidth(-5);
                    ImGui::Indent(5);
                    ImGui::Spacing();

                    ImGui::Text("Join a local game");
                    ImGui::Separator();

                    ImGui::Text(" IP Address:");
                    static char ipBuf[64] = "";
                    strcpy_s(ipBuf, ip->c_str());
                    static Networking::HostStatus hostOnline = Networking::HostStatus::HOST_UNKNOWN;
                    static DestAddrType addressType = getDestAddrType(ipBuf);
                    if (addressType == DestAddrType::UNKNOWN_ADDR) {
                        ImGui::BeginErrorBorder();
                        if (ImGui::InputText("##ip_join", ipBuf, 64)) {
                            addressType = getDestAddrType(ipBuf);
                            if (hostOnline != Networking::HostStatus::HOST_BUSY) {
                                hostOnline = Networking::HostStatus::HOST_UNKNOWN;
                            }
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Unknown address.");
                        }
                        ImGui::EndErrorBorder();
                    }
                    else if (hostOnline == Networking::HostStatus::HOST_ERROR) {
                        ImGui::BeginErrorBorder();
                        if (ImGui::InputText("##ip_join", ipBuf, 64)) {
                            addressType = getDestAddrType(ipBuf);
                            if (hostOnline != Networking::HostStatus::HOST_BUSY) {
                                hostOnline = Networking::HostStatus::HOST_UNKNOWN;
                            }
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("An error occurred while trying to read the host.");
                        }
                        ImGui::EndErrorBorder();
                    }
                    else if (addressType == DestAddrType::INTERAL_ADDR) {
                        ImGui::BeginWarnBorder();
                        if (ImGui::InputText("##ip_join", ipBuf, 64)) {
                            addressType = getDestAddrType(ipBuf);
                            if (hostOnline != Networking::HostStatus::HOST_BUSY) {
                                hostOnline = Networking::HostStatus::HOST_UNKNOWN;
                            }
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Warning: this is not an external IP address");
                        }
                        ImGui::EndWarnBorder();
                    }
                    else if (hostOnline == Networking::HostStatus::HOST_TIMEOUT) {
                        ImGui::BeginWarnBorder();
                        if (ImGui::InputText("##ip_join", ipBuf, 64)) {
                            addressType = getDestAddrType(ipBuf);
                            if (hostOnline != Networking::HostStatus::HOST_BUSY) {
                                hostOnline = Networking::HostStatus::HOST_UNKNOWN;
                            }
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Warning: could not reach host");
                        }
                        ImGui::EndWarnBorder();
                    }
                    else if (hostOnline == Networking::HostStatus::HOST_ONLINE) {
                        ImGui::BeginSuccesBorder();
                        if (ImGui::InputText("##ip_join", ipBuf, 64)) {
                            addressType = getDestAddrType(ipBuf);
                            if (hostOnline != Networking::HostStatus::HOST_BUSY) {
                                hostOnline = Networking::HostStatus::HOST_UNKNOWN;
                            }
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Succesfully reached the host");
                        }
                        ImGui::EndSuccesBorder();
                    }
                    else {
                        if (ImGui::InputText("##ip_join", ipBuf, 64)) {
                            addressType = getDestAddrType(ipBuf);
                        }
                        if (ImGui::IsItemActiveLastFrame() && !ImGui::IsItemActive()) {
                            if (hostOnline == Networking::HostStatus::HOST_UNKNOWN) {
                                Networking::pingHost(ipBuf, (unsigned short)*port, &hostOnline, true);
                            }
                        }
                    }
                    cvarManager->getCvar("mp_ip").setValue(std::string(ipBuf));
                    ImGui::Text(" Port:");
                    static int portBuf = *port;
                    static bool invalidJoinPort = portBuf < 1 || portBuf > 65535;
                    if (invalidJoinPort) {
                        ImGui::BeginErrorBorder();
                        if (ImGui::InputInt("##port_join", &portBuf, 0)) {
                            invalidJoinPort = portBuf < 1 || portBuf > 65535;
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Invalid port");
                        }
                        ImGui::EndErrorBorder();
                    }
                    else {
                        if (ImGui::InputInt("##port_join", &portBuf, 0)) {
                            invalidJoinPort = portBuf < 1 || portBuf > 65535;
                        }
                    }
                    cvarManager->getCvar("mp_port").setValue(portBuf);
                    ImGui::Text(" Password: (optional)");
                    static char pswdBuf[64] = "";
                    ImGui::InputText("##pswd_join", pswdBuf, 64, ImGuiInputTextFlags_Password);
                    ImGui::Separator();

                    ImGui::Indent(-5);
                    ImGui::Checkbox("Joining a custom map", &joinCustomMap);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Select this if you are joining a non Rocket League map.");
                    }
                    if (joinCustomMap) {
                        if (currentJoinMap >= (int)joinableMaps.size()) {
                            currentJoinMap = 0;
                            joinableMaps = getWorkshopMaps(*workshopMapDirPath);
                            std::vector<std::filesystem::path> customMaps = getFilesFromDir(*customMapDirPath, 2, ".upk", ".udk");
                            joinableMaps.insert(std::end(joinableMaps), std::begin(customMaps), std::end(customMaps));
                        }

                        const int input_size = 32;
                        char input_buffer[input_size] = "";
                        std::string preview_value = "No maps found";
                        if ((int)joinableMaps.size() > currentJoinMap) {
                            preview_value = joinableMaps[currentJoinMap].filename().string();
                        }
                        bool buffer_changed = false;
                        if (ImGui::BeginSearchableCombo("##Maps", preview_value.c_str(), input_buffer, input_size, "type to search", &buffer_changed)) {
                            // Only refresh maps on user interaction.
                            if (buffer_changed) {
                                joinableMaps = getWorkshopMaps(*workshopMapDirPath);
                                std::vector<std::filesystem::path> customMaps = getFilesFromDir(*customMapDirPath, 2, ".upk", ".udk");
                                joinableMaps.insert(std::end(joinableMaps), std::begin(customMaps), std::end(customMaps));
                            }

                            int matched_items = 0;
                            std::filesystem::path last_path;
                            for (int i = 0; i < (int)joinableMaps.size(); i++) {
                                char buffer[input_size] = "";
                                ImStrncpy(buffer, input_buffer, input_size);
                                std::string item_text = joinableMaps[i].filename().string();
                                // Allow filtering on file and workshop name.
                                std::string folder_name = joinableMaps[i].parent_path().filename().string();
                                uint64_t workshopMapId = std::atoll(folder_name.c_str());
                                if (subscribedWorkshopMaps.find(workshopMapId) != subscribedWorkshopMaps.end()) {
                                    WorkshopMap workshopMap = subscribedWorkshopMaps[workshopMapId];
                                    if (!workshopMap.title.empty()) {
                                        folder_name = workshopMap.title;
                                    }
                                }

                                if (toLower(item_text).find(toLower(buffer), 0) == std::string::npos && toLower(folder_name).find(toLower(buffer), 0) == std::string::npos) {
                                    continue;
                                }

                                ImGui::PushID((void*)(intptr_t)i);
                                const bool item_selected = (i == currentJoinMap);
                                if (joinableMaps[i].parent_path() != last_path) {
                                    last_path = joinableMaps[i].parent_path();
                                    ImGui::Selectable((">" + folder_name).c_str(), false, ImGuiSelectableFlags_Disabled);
                                }
                                if (ImGui::Selectable(item_text.c_str(), item_selected)) {
                                    currentJoinMap = i;
                                }
                                matched_items++;
                                last_path = joinableMaps[i].parent_path();
                                if (item_selected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                                ImGui::PopID();
                            }
                            if (matched_items == 0) {
                                ImGui::Selectable("No maps found", false, ImGuiSelectableFlags_Disabled);
                            }

                            ImGui::EndCombo();
                        }
                    }
                    ImGui::Separator();

                    ImGui::Indent(5);
                    if (ImGui::Button("Join")) {
                        gameWrapper->Execute([this, pswd = pswdBuf](GameWrapper*) {
                            joinGame(pswd);
                        });
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("In Game Mods")) {
                if (ImGui::BeginChild("##InGameMods", ImVec2(0, 0), true)) {
                    if (ImGui::CollapsingHeader("Game Event Mods")) {
                        ImGui::Indent(20);
                        if (ImGui::CollapsingHeader("Game Controls")) {
                            if (ImGui::Button("Force Overtime")) {
                                gameWrapper->Execute([this](GameWrapper*) {
                                    forceOvertime();
                                });
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Pause Server")) {
                                gameWrapper->Execute([this](GameWrapper*) {
                                    pauseServer();
                                });
                            }
                            if (ImGui::Button("Restart Match")) {
                                gameWrapper->Execute([this](GameWrapper*) {
                                    resetMatch();
                                });
                            }
                            if (ImGui::Button("Reset Players")) {
                                gameWrapper->Execute([this](GameWrapper*) {
                                    resetPlayers();
                                });
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Reset Balls")) {
                                gameWrapper->Execute([this](GameWrapper*) {
                                    resetBalls();
                                });
                            }
                        }
                        if (ImGui::CollapsingHeader("Match Settings")) {
                            int maxPlayers = getMaxPlayers();
                            if (ImGui::InputInt("Max Players", &maxPlayers)) {
                                gameWrapper->Execute([this, newMaxPlayers = maxPlayers](GameWrapper*) {
                                    setMaxPlayers(newMaxPlayers);
                                });
                            }
                            int maxTeamSize = getMaxTeamSize();
                            if (ImGui::InputInt("Max Team Size", &maxTeamSize)) {
                                gameWrapper->Execute([this, newMaxTeamSize = maxTeamSize](GameWrapper*) {
                                    setMaxTeamSize(newMaxTeamSize);
                                });
                            }
                            int respawnTime = getRespawnTime();
                            if (ImGui::InputInt("Respawn Time", &respawnTime)) {
                                gameWrapper->Execute([this, newRespawnTime = respawnTime](GameWrapper*) {
                                    setRespawnTime(newRespawnTime);
                                });
                            }
                            int blueScore = getScoreBlue();
                            ImGui::PushItemWidth(150);
                            if (ImGui::InputInt("Blue Score", &blueScore)) {
                                gameWrapper->Execute([this, newBlueScore = blueScore](GameWrapper*) {
                                    setScoreBlue(newBlueScore);
                                });
                            }
                            ImGui::SameLine();
                            int orangeScore = getScoreOrange();
                            if (ImGui::InputInt("Orange Score", &orangeScore)) {
                                gameWrapper->Execute([this, newOrangeScore = orangeScore](GameWrapper*) {
                                    setScoreOrange(newOrangeScore);
                                });
                            }
                            ImGui::PopItemWidth();
                            int gameTimeRemaining = getGameTimeRemaining();
                            if (ImGui::DragTime("Time Remaining", &gameTimeRemaining)) {
                                gameWrapper->Execute([this, newGameTimeRemaining = gameTimeRemaining](GameWrapper*) {
                                    setGameTimeRemaining(newGameTimeRemaining);
                                });
                            }
                            bool isGoalDelayDisabled = getIsGoalDelayDisabled();
                            if (ImGui::Checkbox("Disable Goal Delay", &isGoalDelayDisabled)) {
                                gameWrapper->Execute([this, isGoalDelayDisabled = isGoalDelayDisabled](GameWrapper*) {
                                    setIsGoalDelayDisabled(isGoalDelayDisabled);
                                });
                            }
                            bool isUnlimitedTime = getIsUnlimitedTime();
                            if (ImGui::Checkbox("Unlimited Time", &isUnlimitedTime)) {
                                gameWrapper->Execute([this, isUnlimitedTime = isUnlimitedTime](GameWrapper*) {
                                    setIsUnlimitedTime(isUnlimitedTime);
                                });
                            }
                        }
                        if (ImGui::CollapsingHeader("Bots")) {
                            if (ImGui::InputInt("# Bots per team", &numBots)) {
                                gameWrapper->Execute([this, newNumBots = numBots](GameWrapper*) {
                                    prepareBots(newNumBots);
                                });
                            }
                            bool isAutofilledWithBots = getIsAutofilledWithBots();
                            if (ImGui::Checkbox("Autofill with bots", &isAutofilledWithBots)) {
                                gameWrapper->Execute([this, isAutofilledWithBots = isAutofilledWithBots](GameWrapper*) {
                                    setIsAutofilledWithBots(isAutofilledWithBots);
                                });
                            }
                            ImGui::SameLine();
                            bool isUnfairTeams = getIsUnfairTeams();
                            if (ImGui::Checkbox("Unfair Teams", &isUnfairTeams)) {
                                gameWrapper->Execute([this, isUnfairTeams = isUnfairTeams](GameWrapper*) {
                                    setIsUnfairTeams(isUnfairTeams);
                                });
                            }
                            if (ImGui::Button("Freeze Bots")) {
                                gameWrapper->Execute([this](GameWrapper*) {
                                    freezeBots();
                                });
                            }
                        }
                        ImGui::Unindent();
                    }
                    if (ImGui::CollapsingHeader("Ball Mods")) {
                        int numBalls = getNumBalls();
                        if (ImGui::InputInt("# Balls", &numBalls)) {
                            gameWrapper->Execute([this, newNumBalls = numBalls](GameWrapper*) {
                                setNumBalls(newNumBalls);
                            });
                        }
                        float ballScale = getBallsScale();
                        if (ImGui::SliderFloat("Balls Scale", &ballScale, 0.1f, 10.0f, "%.1fX")) {
                            gameWrapper->Execute([this, newBallScale = ballScale](GameWrapper*) {
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
                            ImGui::Text("Player");
                            ImGui::NextColumn();
                            ImGui::Text("Admin");
                            ImGui::NextColumn();
                            ImGui::Text("Hidden");
                            ImGui::NextColumn();
                            ImGui::NextColumn();
                            ImGui::Separator();
                            for (PriWrapper& player : players) {
                                std::string displayName = player.GetPlayerName().ToString();
                                ImGui::Text(displayName.c_str());
                                ImGui::NextColumn();
                                bool isAdmin = getIsAdmin(player);
                                if (ImGui::Checkbox(("##Admin" + player.GetPlayerName().ToString()).c_str(), &isAdmin)) {
                                    gameWrapper->Execute([this, player = player, isAdmin = isAdmin](GameWrapper*) {
                                        setIsAdmin(player, isAdmin);
                                    });
                                }
                                ImGui::NextColumn();
                                bool isHidden = getIsHidden(player);
                                if (ImGui::Checkbox(("##Hidden" + player.GetPlayerName().ToString()).c_str(), &isHidden)) {
                                    gameWrapper->Execute([this, player = player, isHidden = isHidden](GameWrapper*) {
                                        setIsHidden(player, isHidden);
                                    });
                                }
                                ImGui::NextColumn();
                                if (ImGui::Button(("Demolish##" + player.GetPlayerName().ToString()).c_str())) {
                                    gameWrapper->Execute([this, player = player](GameWrapper*) {
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
                        std::vector<PriWrapper> players = getPlayers(true);
                        std::vector<std::string> playersNames = getPlayersNames(players);
                        ImGui::Text("Modify: ");
                        ImGui::SameLine();
                        ImGui::Combo("##Players", &selectedPlayer, playersNames, "No players found");
                        ImGui::Separator();

                        PriWrapper player = getPlayerFromPlayers(players, selectedPlayer);
                        CarPhysics playerPhysics = getPhysics(player);
                        if (ImGui::SliderFloat("Car Scale", &playerPhysics.carScale, 0.1f, 2.0f, "%.1fX")) {
                            gameWrapper->Execute([this, player = player, newCarScale = playerPhysics.carScale](GameWrapper*) {
                                setCarScale(player, newCarScale);
                            });
                        }
                        if (ImGui::Checkbox("Car Collision", &playerPhysics.carHasCollision)) {
                            gameWrapper->Execute([this, player = player, carHasCollision = playerPhysics.carHasCollision](GameWrapper*) {
                                setbCarCollision(player, carHasCollision);
                            });
                        }
                        ImGui::SameLine();
                        if (ImGui::Checkbox("Freeze car", &playerPhysics.carIsFrozen)) {
                            gameWrapper->Execute([this, player = player, carIsFrozen = playerPhysics.carIsFrozen](GameWrapper*) {
                                setCarIsFrozen(player, carIsFrozen);
                            });
                        }
                        ImGui::Separator();

                        if (ImGui::DragFloat("Torque Rate", &playerPhysics.torqueRate, 0.1f, 0.0f, 0.0f, "%.3fx10^5 N*m")) {
                            gameWrapper->Execute([this, player = player, torqueRate = playerPhysics.torqueRate](GameWrapper*) {
                                setTorqueRate(player, torqueRate);
                            });
                        }
                        if (ImGui::DragFloat("Max Car Velocity", &playerPhysics.maxCarVelocity, 1.0f, 0.0f, 0.0f, "%.3f u/s")) {
                            gameWrapper->Execute([this, player = player, maxCarVelocity = playerPhysics.maxCarVelocity](GameWrapper*) {
                                setMaxCarVelocity(player, maxCarVelocity);
                            });
                        }
                        if (ImGui::DragFloat("Ground Sticky Force", &playerPhysics.groundStickyForce, 1.0f, 0.0f, 0.0f, "%.3f N")) {
                            gameWrapper->Execute([this, player = player, groundStickyForce = playerPhysics.groundStickyForce](GameWrapper*) {
                                setGroundStickyForce(player, groundStickyForce);
                            });
                        }
                        if (ImGui::DragFloat("Wall Sticky Force", &playerPhysics.wallStickyForce, 1.0f, 0.0f, 0.0f, "%.3f N")) {
                            gameWrapper->Execute([this, player = player, wallStickyForce = playerPhysics.wallStickyForce](GameWrapper*) {
                                setWallStickyForce(player, wallStickyForce);
                            });
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Gamemodes")) {
                if (ImGui::BeginChild("##GamemodesList", ImVec2(200, 0), true)) {
                    ImGui::Text("available gamemodes:");
                    ImGui::Separator();
                    for (auto i = 0u; i < gamemodes.size(); i++) {
                        std::shared_ptr<RocketGameMode>& gamemode = gamemodes[i];
                        if (ImGui::SwitchCheckbox(("##" + gamemode->GetGamemodeName() + std::to_string(i)).c_str(), gamemode->IsActive())) {
                            gameWrapper->Execute([this, gamemode = gamemode](GameWrapper*) {
                                gamemode->Activate(!gamemode->IsActive());
                            });
                        }
                        ImGui::SameLine();
                        float backup_button_text_align_x = ImGui::GetStyle().ButtonTextAlign.x;
                        float backup_frame_rounding = ImGui::GetStyle().FrameRounding;
                        ImGui::GetStyle().ButtonTextAlign.x = 0;
                        ImGui::GetStyle().FrameRounding = 3;
                        if (ImGui::ButtonEx(gamemode->GetGamemodeName().c_str(), ImVec2(-1, 0), ImGuiButtonFlags_AlignTextBaseLine)) {
                            gamemodeSelected = i;
                        }
                        ImGui::GetStyle().ButtonTextAlign.x = backup_button_text_align_x;
                        ImGui::GetStyle().FrameRounding = backup_frame_rounding;
                    }
                }
                ImGui::EndChild();
                ImGui::SameLine();
                if (ImGui::BeginChild("##GamemodesOptions", ImVec2(0, 0), true)) {
                    if (gamemodeSelected < (int)gamemodes.size()) {
                        std::shared_ptr<RocketGameMode> gamemode = gamemodes[gamemodeSelected];
                        ImGui::Text(gamemode->GetGamemodeName().c_str());
                        ImGui::Separator();

                        if (gamemode->IsActive()) {
                            gamemode->RenderOptions();
                        }
                        else {
                            ImGui::BeginDisabled();
                            gamemode->RenderOptions();
                            ImGui::EndDisabled();
                        }
                    }
                    else {
                        ImGui::Text("No gamemodes found.");
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
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
void RocketPlugin::SetImGuiContext(uintptr_t ctx)
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
