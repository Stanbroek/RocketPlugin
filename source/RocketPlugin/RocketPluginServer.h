#pragma once
#include "RocketPlugin.h"

#define STEAM_API_NODLL
#include "SteamWorksWrapper/steam_gameserver.h"


class RocketPluginServer
{
public:
    RocketPluginServer() = default;
    ~RocketPluginServer() { Stop(); }
    RocketPluginServer(const RocketPluginServer&) = delete;
    RocketPluginServer(RocketPluginServer&&) = delete;
    RocketPluginServer& operator=(const RocketPluginServer&) = delete;
    RocketPluginServer& operator=(RocketPluginServer&&) = delete;

    void Start() const;
    void Stop() const;
    
    void SetSettings() const;
    void SetRichPresence() const;
    void LogStatus() const;

    bool connectedToSteam = false;
    bool loggedInToSteam = false;

    int maxPlayerCount = 8;
    bool passwordProtected = false;
    std::string serverName = "Such Server Many Wow";
    std::string mapName = "Galleon";
    std::string gameTags = "RocketPluginServer";

private:
    // BUG, Log: Don't use 'IsSteamSocketsServer' on non-listen-server clients; crashing

    /*
     * Various callback functions that Steam will call to let us know about events related to our
     * connection to the Steam servers for authentication purposes.
     */

    // Tells us when we have successfully connected to Steam
    STEAM_GAMESERVER_CALLBACK(RocketPluginServer, OnSteamServersConnected, SteamServersConnected_t);

    // Tells us when there was a failure to connect to Steam
    STEAM_GAMESERVER_CALLBACK(RocketPluginServer, OnSteamServersConnectFailure, SteamServerConnectFailure_t);

    // Tells us when we have been logged out of Steam
    STEAM_GAMESERVER_CALLBACK(RocketPluginServer, OnSteamServersDisconnected, SteamServersDisconnected_t);

    // Tells us that Steam has set our security policy (VAC on or off)
    STEAM_GAMESERVER_CALLBACK(RocketPluginServer, OnPolicyResponse, GSPolicyResponse_t);

    /*
     * Various callback functions that Steam will call to let us know about whether we should
     * allow clients to play or we should kick/deny them.
     */

    // Tells us a client has been authenticated and approved to play by Steam (passes auth, license check, VAC status, etc...)
    STEAM_GAMESERVER_CALLBACK(RocketPluginServer, OnValidateAuthTicketResponse, ValidateAuthTicketResponse_t);

    // client connection state
    STEAM_GAMESERVER_CALLBACK(RocketPluginServer, OnP2PSessionRequest, P2PSessionRequest_t);
    STEAM_GAMESERVER_CALLBACK(RocketPluginServer, OnP2PSessionConnectFail, P2PSessionConnectFail_t);


//#define STEAM_GAMESERVER_CALLBACK_STUB(thisclass, param) \
//    STEAM_GAMESERVER_CALLBACK(thisclass, OnGS ## param, param) { pParam; TRACE_LOG("steam gameserver callback stub"); }
//#define STEAM_CALLBACK_STUB(thisclass, param) \
//    STEAM_CALLBACK(thisclass, On ## param, param) { pParam; TRACE_LOG("steam callback stub"); }
//#define CALLBACK_STUB(thisclass, param) \
//	STEAM_CALLBACK_STUB(thisclass, param) \
//	STEAM_GAMESERVER_CALLBACK_STUB(thisclass, param)
//
//    /// isteammatchmakingservers.h callbacks:
//    CALLBACK_STUB(RocketPluginServer, FavoritesListChanged_t)
//    CALLBACK_STUB(RocketPluginServer, LobbyInvite_t)
//    CALLBACK_STUB(RocketPluginServer, LobbyEnter_t)
//    CALLBACK_STUB(RocketPluginServer, LobbyDataUpdate_t)
//    CALLBACK_STUB(RocketPluginServer, LobbyChatUpdate_t)
//    CALLBACK_STUB(RocketPluginServer, LobbyChatMsg_t)
//    CALLBACK_STUB(RocketPluginServer, LobbyGameCreated_t)
//    CALLBACK_STUB(RocketPluginServer, LobbyMatchList_t)
//    CALLBACK_STUB(RocketPluginServer, LobbyKicked_t)
//    CALLBACK_STUB(RocketPluginServer, LobbyCreated_t)
//    CALLBACK_STUB(RocketPluginServer, PSNGameBootInviteResult_t)
//    CALLBACK_STUB(RocketPluginServer, FavoritesListAccountsUpdated_t)
//
//    /// isteamfriends.h callbacks:
//    CALLBACK_STUB(RocketPluginServer, GameRichPresenceJoinRequested_t)
//	
//    /// isteamclient.h callbacks:
//    CALLBACK_STUB(RocketPluginServer, SteamServersConnected_t)
//    CALLBACK_STUB(RocketPluginServer, SteamServerConnectFailure_t)
//    CALLBACK_STUB(RocketPluginServer, SteamServersDisconnected_t)
//    CALLBACK_STUB(RocketPluginServer, ClientGameServerDeny_t)
//    CALLBACK_STUB(RocketPluginServer, IPCFailure_t)
//    CALLBACK_STUB(RocketPluginServer, LicensesUpdated_t)
//    CALLBACK_STUB(RocketPluginServer, ValidateAuthTicketResponse_t)
//    CALLBACK_STUB(RocketPluginServer, MicroTxnAuthorizationResponse_t)
//    CALLBACK_STUB(RocketPluginServer, EncryptedAppTicketResponse_t)
//    CALLBACK_STUB(RocketPluginServer, GetAuthSessionTicketResponse_t)
//    CALLBACK_STUB(RocketPluginServer, GameWebCallback_t)
//    CALLBACK_STUB(RocketPluginServer, StoreAuthURLResponse_t)
//
//	/// isteamgameserver.h callbacks
//    CALLBACK_STUB(RocketPluginServer, GSClientApprove_t)
//    CALLBACK_STUB(RocketPluginServer, GSClientDeny_t)
//    CALLBACK_STUB(RocketPluginServer, GSClientKick_t)
//    CALLBACK_STUB(RocketPluginServer, GSClientAchievementStatus_t)
//    CALLBACK_STUB(RocketPluginServer, GSPolicyResponse_t)
//    CALLBACK_STUB(RocketPluginServer, GSGameplayStats_t)
//    CALLBACK_STUB(RocketPluginServer, GSClientGroupStatus_t)
//    CALLBACK_STUB(RocketPluginServer, GSReputation_t)
//    CALLBACK_STUB(RocketPluginServer, AssociateWithClanResult_t)
//    CALLBACK_STUB(RocketPluginServer, ComputeNewPlayerCompatibilityResult_t)
//
//	/// isteamnetworking.h callbacks
//    CALLBACK_STUB(RocketPluginServer, P2PSessionRequest_t)
//    CALLBACK_STUB(RocketPluginServer, P2PSessionConnectFail_t)
//    CALLBACK_STUB(RocketPluginServer, SocketStatusCallback_t)
};
