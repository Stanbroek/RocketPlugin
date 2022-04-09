#pragma once
#include "Modules/RocketPluginModule.h"


typedef uint8_t uint6_t;
typedef uint6_t NetId;

constexpr int INVALID_PLAYER_ID = -1;
constexpr NetId INVALID_NET_ID = 0;
constexpr uint6_t RP_MESSAGE_VERSION = 1;
constexpr const char* RP_MESSAGE_SIGNATURE = "RP";
constexpr size_t RP_MESSAGE_SIGNATURE_SIZE = std::string_view(RP_MESSAGE_SIGNATURE).length();
constexpr size_t RP_MESSAGE_HEADER_SIZE = 10;


struct RPMessageHeader
{
    RPMessageHeader() = default;
    RPMessageHeader(const NetId moduleId, const int senderId, const int receiverId)
        : ModuleId(moduleId), SenderId(senderId), ReceiverId(receiverId) {}

    std::string Serialize() const;
    void Deserialize(const std::string& message);

    uint6_t Version = RP_MESSAGE_VERSION;
    NetId ModuleId = INVALID_NET_ID;
    int SenderId = INVALID_PLAYER_ID;
    int ReceiverId = INVALID_PLAYER_ID;

protected:
    size_t headerSize = 0;
};


struct RPMessage : RPMessageHeader
{
    RPMessage() = default;
    RPMessage(const RPMessageHeader& header, std::string message)
        : RPMessageHeader(header), Message(std::move(message)) {}

    std::string Serialize() const;
    void Deserialize(const std::string& message);

    std::string Message;
};


class RPNetCode;

class NetworkedModule
{
    friend RPNetCode;
public:
    NetworkedModule();
    virtual ~NetworkedModule() = default;

    bool SendMessageTo(PriWrapper pri, const std::string& message) const;
    bool BroadcastMessage(const std::string& message) const;
    virtual void Receive(PriWrapper sender, const std::string& message) = 0;

    bool networked = false;
protected:
    const NetId netId = INVALID_NET_ID;
};


class RPNetCode final : RocketPluginModule
{
    friend NetworkedModule;
public:
    void Init();
    NetId Register(NetworkedModule* networkedModule);
    void Deregister(NetId netId);

protected:
    bool send(const RPMessage& message);

private:
    bool sendClient(const PlayerControllerWrapper& player, const std::string& serializedMessage);
    bool sendServer(const PlayerControllerWrapper& player, const std::string& serializedMessage);
    void receive(const std::string& serializedMessage);

    size_t messagesSend = 0;
    size_t messagesReceived = 0;
    std::vector<std::pair<NetworkedModule*, bool>> registeredModules;
};
