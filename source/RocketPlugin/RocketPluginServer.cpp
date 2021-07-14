// RocketPluginServer.cpp
// Custom server for the RocketPlugin plugin.
//
// Author:       Stanbroek
// Version:      0.6.5 05/02/21

#include "RocketPluginServer.h"

#include <winsock.h>

void RocketPluginServer::Start() const
{
    [[maybe_unused]] const char* gameDir = "rocketleague";
    [[maybe_unused]] const uint32 ip = INADDR_ANY;
    [[maybe_unused]] const uint16 masterServerUpdaterPort = MASTERSERVERUPDATERPORT_USEGAMESOCKETSHARE;
    [[maybe_unused]] const EServerMode mode = eServerModeNoAuthentication;  // Don't authenticate user logins and don't list on the server list
    //const EServerMode mode = eServerModeAuthentication;  // Authenticate users, list on the server list, don't run VAC on clients that connect

    if (SteamGameServer() == nullptr) {
        TRACE_LOG("initializing Steam game server interface");

        if (!SteamGameServer_Init(ip, 8766, 27815, 27816, mode, "0.0.2")) {
            ERROR_LOG("SteamGameServer_Init call failed");
        }
        if (SteamGameServer() == nullptr) {
            ERROR_LOG("Steam game server interface interface is invalid");
            return;
        }
    }
    else {
        TRACE_LOG("Steam game server interface is already Initialized");
    }

    if (!SteamGameServer()->BLoggedOn()) {
        TRACE_LOG("logging in Steam game server");
        SteamGameServer()->SetModDir(gameDir);
        SteamGameServer()->SetProduct("252950");
        SteamGameServer()->SetGameDescription("Rocket League");
        SteamGameServer()->LogOnAnonymous();
        SteamGameServer()->EnableHeartbeats(true);
    }
    else {
        TRACE_LOG("Steam game server is already logged on");
    }
}


void RocketPluginServer::Stop() const
{
    if (connectedToSteam) {
        TRACE_LOG("logging off Steam game server");
        SteamGameServer()->LogOff();
    }

    //SteamGameServer_Shutdown();
}


void RocketPluginServer::SetSettings() const
{
    if (SteamGameServer() != nullptr) {
        SteamGameServer()->SetMaxPlayerCount(maxPlayerCount);
        SteamGameServer()->SetPasswordProtected(passwordProtected);
        SteamGameServer()->SetServerName(serverName.c_str());
        SteamGameServer()->SetMapName(mapName.c_str());
        SteamGameServer()->SetGameTags(gameTags.c_str());
        TRACE_LOG("set Steam game server settings");
    }
}


void RocketPluginServer::SetRichPresence() const
{
    if (SteamGameServer() && SteamUser() && SteamFriends()) {
        const u_long addr = htonl(SteamGameServer()->GetPublicIP());
        const std::string value = fmt::format("-SteamConnectID={} -SteamConnectIP={}",
            SteamUser()->GetSteamID().ConvertToUint64(), Networking::IPv4ToString(&addr));
        if (SteamFriends()->SetRichPresence("connect", value.c_str())) {
            TRACE_LOG("set rich presence to connect:{}", quote(value));
        }
        else {
            TRACE_LOG("failed to set rich presence to connect:{}", quote(value));
        }
    }
}


void RocketPluginServer::LogStatus() const
{
    
    TRACE_LOG("Connected to Steam: {}\n", connectedToSteam);
    TRACE_LOG("Logged in to Steam: {}\n", loggedInToSteam);
    ISteamGameServer* steamGameServer = SteamGameServer();
    if (steamGameServer != nullptr) {
        TRACE_LOG("Logged On: {}\n", steamGameServer->BLoggedOn());
        TRACE_LOG("Secure: {}\n", steamGameServer->BSecure());
        TRACE_LOG("Steam ID: {}\n", steamGameServer->GetSteamID().ConvertToUint64());
        const u_long addr = htonl(steamGameServer->GetPublicIP());
        TRACE_LOG("public IP: {}", Networking::IPv4ToString(&addr));
    }
    else {
        TRACE_LOG("Error, could not get Steam game server");
    }
}


//-----------------------------------------------------------------------------
// Purpose: called when a connections to the Steam back-end has been established
//			this means the Steam client now has a working connection to the Steam servers
//			usually this will have occurred before the game has launched, and should
//			only be seen if the user has dropped connection due to a networking issue
//			or a Steam server update
//-----------------------------------------------------------------------------
void RocketPluginServer::OnSteamServersConnected([[maybe_unused]] SteamServersConnected_t* pLogonSuccess)
{
    TRACE_LOG("succesfully");
    connectedToSteam = true;
    SetSettings();
}


//-----------------------------------------------------------------------------
// Purpose: called when a connection attempt has failed
//			this will occur periodically if the Steam client is not connected, 
//			and has failed in it's retry to establish a connection
//-----------------------------------------------------------------------------
void RocketPluginServer::OnSteamServersConnectFailure([[maybe_unused]] SteamServerConnectFailure_t* pConnectFailure)
{
    TRACE_LOG("Result: {:X}, Still Retrying: {}", pConnectFailure->m_eResult, pConnectFailure->m_bStillRetrying);
    connectedToSteam = false;
}


//-----------------------------------------------------------------------------
// Purpose: called if the client has lost connection to the Steam servers
//			real-time services will be disabled until a matching SteamServersConnected_t has been posted
//-----------------------------------------------------------------------------
void RocketPluginServer::OnSteamServersDisconnected([[maybe_unused]] SteamServersDisconnected_t* pLoggedOff)
{
    TRACE_LOG("Result: {:X}", pLoggedOff->m_eResult);
    connectedToSteam = false;
}


//-----------------------------------------------------------------------------
// Purpose: received when the game server requests to be displayed as secure (VAC protected)
//          m_bSecure is true if the game server should display itself as secure to users, false otherwise
//-----------------------------------------------------------------------------
void RocketPluginServer::OnPolicyResponse([[maybe_unused]] GSPolicyResponse_t* pPolicyResponse)
{
    TRACE_LOG("Secure: {}", static_cast<bool>(pPolicyResponse->m_bSecure));
    loggedInToSteam = true;
}


//-----------------------------------------------------------------------------
// callback for BeginAuthSession
//-----------------------------------------------------------------------------
void RocketPluginServer::OnValidateAuthTicketResponse(ValidateAuthTicketResponse_t* pResponse)
{
    TRACE_LOG("from {}", pResponse->m_SteamID.ConvertToUint64());
    if (pResponse->m_eAuthSessionResponse == k_EAuthSessionResponseOK) {
        INFO_LOG("authenticated");
        //SteamGameServerNetworking()->SendP2PPacket(pResponse->m_SteamID, nullptr, NULL, k_EP2PSendUnreliable);
    }
    else {
        INFO_LOG("failed to authenticate, {:X}", pResponse->m_eAuthSessionResponse);
        //SteamGameServerNetworking()->SendP2PPacket(pResponse->m_SteamID, nullptr, NULL, k_EP2PSendReliable);
    }
}


//-----------------------------------------------------------------------------
// Purpose: callback notification - a user wants to talk to us over the P2P channel via the SendP2PPacket() API
//          in response, a call to AcceptP2PPacketsFromUser() needs to be made, if you want to talk with them
//-----------------------------------------------------------------------------
void RocketPluginServer::OnP2PSessionRequest(P2PSessionRequest_t* pCallback)
{
    TRACE_LOG("from {}", pCallback->m_steamIDRemote.ConvertToUint64());
    // We'll accept a connection from anyone.
    SteamGameServerNetworking()->AcceptP2PSessionWithUser(pCallback->m_steamIDRemote);
}


//-----------------------------------------------------------------------------
// Purpose: callback notification - packets can't get through to the specified user via the SendP2PPacket() API
//          all packets queued packets unsent at this point will be dropped
//          further attempts to send will retry making the connection (but will be dropped if we fail again)
//-----------------------------------------------------------------------------
void RocketPluginServer::OnP2PSessionConnectFail([[maybe_unused]] P2PSessionConnectFail_t* pCallback)
{
    TRACE_LOG("from {}", pCallback->m_steamIDRemote.ConvertToUint64());
    INFO_LOG("dropped connection");
}