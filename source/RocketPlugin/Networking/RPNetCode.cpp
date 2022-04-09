#include "RPNetCode.h"

#include "RocketPlugin.h"


const std::string base64array = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/// <summary>Encodes a single value to base64.</summary>
/// <param name="value">Value to convert</param>
/// <returns>Base64 character</returns>
char encode_base64(const uint6_t value)
{
    return base64array[value & 0x3F];
}


/// <summary>Decodes a single value from base64.</summary>
/// <param name="value">Base64 character</param>
/// <returns>Decoded value</returns>
uint6_t decode_base64(const char value)
{
    return static_cast<uint6_t>(base64array.find(value));
}


/// <summary>Serializes the message header.</summary>
/// <returns>Serialized message header</returns>
std::string RPMessageHeader::Serialize() const
{
    BitBinaryWriter<unsigned char> bbw(RP_MESSAGE_HEADER_SIZE);
    bbw.WriteBits(Version);
    bbw.WriteBits(ModuleId);
    bbw.WriteBits(SenderId);
    bbw.WriteBits(ReceiverId);

    const std::string serializedHeader = bbw.ToHex();
    return encode_base64(static_cast<uint6_t>(serializedHeader.length() + 1)) + serializedHeader;
}


/// <summary>Deserializes the message header.</summary>
void RPMessageHeader::Deserialize(const std::string& message)
{
    headerSize = decode_base64(message.front());
    if (message.length() < headerSize) {
        throw std::range_error("invalid message size");
    }

    BitBinaryReader<unsigned char> bbr(message.substr(1));
    Version = bbr.ReadBits<uint6_t>();
    ModuleId = bbr.ReadBits<NetId>();
    SenderId = bbr.ReadBits<int>();
    ReceiverId = bbr.ReadBits<int>();
}


/// <summary>Deserializes the message.</summary>
/// <returns>Serialized message</returns>
std::string RPMessage::Serialize() const
{
    return RPMessageHeader::Serialize() + Message;
}


/// <summary>Deserializes the message.</summary>
void RPMessage::Deserialize(const std::string& message)
{
    RPMessageHeader::Deserialize(message);
    Message = message.substr(headerSize);
}


/// <summary>Initializes the networked module.</summary>
NetworkedModule::NetworkedModule()
    : netId(RocketPluginModule::Outer()->netCode.Register(this))
{}


/// <summary>Sends a message to the given pri.</summary>
/// <param name="pri">Pri to send the message to</param>
/// <param name="message">Message to send</param>
/// <returns>Bool with if the message was send successfully</returns>
bool NetworkedModule::SendMessageTo(PriWrapper pri, const std::string& message) const
{
    ServerWrapper game = RocketPluginModule::Outer()->GetGame(true);
    BMCHECK(game, false);

    ControllerWrapper localPlayerController = game.GetLocalPrimaryPlayer().memory_address;
    BMCHECK(localPlayerController, false);

    PlayerReplicationInfoWrapper localPri = localPlayerController.GetPlayerReplicationInfo();
    BMCHECK(localPri, false);

    const RPMessageHeader header(netId, localPri.GetPlayerID(), pri.GetPlayerID());
    return RocketPluginModule::Outer()->netCode.send(RPMessage(header, message));
}


/// <summary>Broadcast a message to all players.</summary>
/// <param name="message">Message to broadcast</param>
/// <returns>Bool with if the message was broadcasted successfully</returns>
bool NetworkedModule::BroadcastMessage(const std::string& message) const
{
    ServerWrapper game = RocketPluginModule::Outer()->GetGame(true);
    BMCHECK(game, false);

    ControllerWrapper localPlayerController = game.GetLocalPrimaryPlayer().memory_address;
    BMCHECK(localPlayerController, false);

    PlayerReplicationInfoWrapper localPri = localPlayerController.GetPlayerReplicationInfo();
    BMCHECK(localPri, false);

    bool sendSuccessful = true;
    for (PriWrapper pri : game.GetPRIs()) {
        BMCHECK_LOOP(pri);
        if (localPri.GetPlayerID() == pri.GetPlayerID()) {
            continue;
        }
        const RPMessageHeader header(netId, localPri.GetPlayerID(), pri.GetPlayerID());
        sendSuccessful &= RocketPluginModule::Outer()->netCode.send(RPMessage(header, message));
    }

    return sendSuccessful;
}


/// <summary></summary>
void RPNetCode::Init()
{
    BM_WARNING_LOG("redacted function");
}


/// <summary>Register a networked module</summary>
/// <param name="networkedModule">Networked module to register</param>
/// <returns>NetId for the registered networked module</returns>
NetId RPNetCode::Register(NetworkedModule* networkedModule)
{
    for (auto& [netModule, enabled] : registeredModules) {
        if (netModule == networkedModule) {
            enabled = true;
            return netModule->netId;
        }
    }

    registeredModules.push_back(std::make_pair(networkedModule, true));

    return static_cast<uint8_t>(registeredModules.size());
}


/// <summary>Deregister a networked module</summary>
/// <param name="netId">NetId of the networked module to deregister</param>
void RPNetCode::Deregister(const NetId netId)
{
    registeredModules.at(netId).second = false;
}


/// <summary>Gets the owner of the given pri.</summary>
/// <param name="pri">Pri to get the owner from</param>
/// <returns>The owner of the pri</returns>
PlayerControllerWrapper get_owner(PriWrapper& pri)
{
    BMCHECK(pri, NULL);

    return PlayerControllerWrapper(pri.GetOwner().memory_address);
}


/// <summary>Gets the owner of the player with the given player id.</summary>
/// <param name="playerId">Player id of the player to get the owner from</param>
/// <returns>The owner of the player with the given player id</returns>
PlayerControllerWrapper get_owner(const int playerId)
{
    ServerWrapper game = RocketPluginModule::Outer()->GetGame(true);
    BMCHECK(game, NULL);

    for (PriWrapper pri : game.GetPRIs()) {
        BMCHECK_LOOP(pri);

        if (pri.GetPlayerID() == playerId) {
            return get_owner(pri);
        }
    }

    return NULL;
}


/// <summary>Gets the pri of the player with the given player id.</summary>
/// <param name="playerId">Player id of the player to get the pri from</param>
/// <returns>The pri of the player with the given player id</returns>
PriWrapper get_pri(const int playerId)
{
    ServerWrapper game = RocketPluginModule::Outer()->GetGame(true);
    BMCHECK(game, NULL);

    for (PriWrapper pri : game.GetPRIs()) {
        BMCHECK_LOOP(pri);

        if (pri.GetPlayerID() == playerId) {
            return pri;
        }
    }

    return NULL;
}


/// <summary>Sends the given message.</summary>
/// <param name="message">Message to send</param>
/// <returns>Bool with if the message was send successfully</returns>
bool RPNetCode::send(const RPMessage& message)
{
    ServerWrapper game = Outer()->GetGame(true);
    BMCHECK(game, false);

    PlayerControllerWrapper localPlayerController = game.GetLocalPrimaryPlayer();
    BMCHECK(localPlayerController, false);

    PlayerControllerWrapper receiversController = get_owner(message.ReceiverId);
    // If we know the owner, we can send them a direct message.
    if (!receiversController.IsNull()) {
        return sendClient(receiversController, message.Serialize());
    }

    // Otherwise we ask the server to redirect our message.
    return sendServer(localPlayerController, message.Serialize());
}


/// <summary>Sends the given message to a other client.</summary>
/// <param name="player">Player to send the message to</param>
/// <param name="serializedMessage">Message to send</param>
/// <returns>Bool with if the message was send successfully</returns>
bool RPNetCode::sendClient(const PlayerControllerWrapper& player, const std::string& serializedMessage)
{
    BM_WARNING_LOG("redacted function");

    return false;
}


/// <summary>Sends the given message to a other client through the server.</summary>
/// <param name="player">Player to send the message to</param>
/// <param name="serializedMessage">Message to send</param>
/// <returns>Bool with if the message was send successfully</returns>
bool RPNetCode::sendServer(const PlayerControllerWrapper& player, const std::string& serializedMessage)
{
    BM_WARNING_LOG("redacted function");

    return false;
}


/// <summary>Gets called when a new message is received.</summary>
/// <param name="serializedMessage">Message that was received.</param>
void RPNetCode::receive(const std::string& serializedMessage)
{
    if (!serializedMessage.starts_with(RP_MESSAGE_SIGNATURE)) {
        BM_TRACE_LOG("Received non Rocket Plugin message {}.", quote(serializedMessage));
        return;
    }

    ++messagesReceived;

    ServerWrapper game = Outer()->GetGame(true);
    BMCHECK(game);

    PlayerControllerWrapper localController = game.GetLocalPrimaryPlayer();
    BMCHECK(localController);

    PriWrapper localPri = localController.GetPRI();
    BMCHECK(localPri);

    RPMessage message;
    message.Deserialize(serializedMessage.substr(RP_MESSAGE_SIGNATURE_SIZE));

    if (message.Version != RP_MESSAGE_VERSION) {
        BM_CRITICAL_LOG("RPMessage version mismatch {:d} != {:d}", message.Version, RP_MESSAGE_VERSION);
        return;
    }

    if (message.SenderId == localPri.GetPlayerID()) {
        BM_TRACE_LOG("received own message, skipping");
        return;
    }

    BM_TRACE_LOG("Received {:d}->{:d}, {:d}: {:d}", message.SenderId, message.ReceiverId, message.ModuleId, message.Version);

    // If the message was not meant for us send it forward.
    if (message.ReceiverId != localPri.GetPlayerID()) {
        PlayerControllerWrapper receiversController = NULL;
        for (PriWrapper pri : game.GetPRIs()) {
            BMCHECK_LOOP(pri);

            if (pri.GetPlayerID() == message.ReceiverId) {
                receiversController = get_owner(pri);
            }
        }
        sendClient(receiversController, serializedMessage);
        return;
    }

    if (registeredModules.size() > message.ModuleId) {
        BM_ERROR_LOG("Could not find module {:d}", message.ModuleId);
        return;
    }

    const auto& [netModule, enabled] = registeredModules.at(message.ModuleId - 1);
    if (enabled) {
        netModule->Receive(get_pri(message.SenderId), message.Message);
    }
}
