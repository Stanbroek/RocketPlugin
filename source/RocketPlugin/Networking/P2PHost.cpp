// P2PHost.cpp
// UDP hole punching using STUN for Rocket Plugin.
//
// Author:       Stanbroek
// Version:      0.6.8 18/09/21
//
// References:
//  https://en.wikipedia.org/wiki/UDP_hole_punching
//  https://www.ietf.org/rfc/rfc3489.txt
//  https://www.ietf.org/rfc/rfc5389.txt

#include "Networking.h"
#include "RocketPlugin.h"

#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "utils/win32_error_category.h"

constexpr timeval NETWORK_TIMEOUT = { 3, 0 };
#define STUN_SERVICES_FILE_PATH     (RocketPluginDataFolder / "STUN-services.txt")

/* Determining STUN message types */
#define IS_REQUEST(msg_type)        (((msg_type) & 0x0110) == 0x0000)
#define IS_INDICATION(msg_type)     (((msg_type) & 0x0110) == 0x0010)
#define IS_SUCCESS_RESP(msg_type)   (((msg_type) & 0x0110) == 0x0100)
#define IS_ERR_RESP(msg_type)       (((msg_type) & 0x0110) == 0x0110)

/* Magic cookie */
#define MAGIC_COOKIE        0x2112A442
#define MAGIC_COOKIE_END    (MAGIC_COOKIE >> 16)
#define MAGIC_COOKIE_BEGIN  (MAGIC_COOKIE & 0xffff)

/* Address family */
#define IPV4 0x01
#define IPV6 0x02

/* STUN message types */
#define BIND_REQUEST_MSG                    0x0001
#define BIND_RESPONSE_MSG                   0x0101
#define BIND_ERROR_RESPONSE_MSG             0x0111
#define SHARED_SECRET_REQUEST_MSG           0x0002
#define SHARED_SECRET_RESPONSE_MSG          0x0102
#define SHARED_SECRET_ERROR_RESPONSE_MSG    0x0112

/* STUN attributes types */
// Comprehension-required range (0x0000-0x7FFF):
#define MAPPED_ADDRESS      0x0001
#define RESPONSE_ADDRESS    0x0002 // Deprecated in [RFC5389]
#define CHANGE_REQUEST      0x0003 // Deprecated in [RFC5389]
#define SOURCE_ADDRESS      0x0004 // Deprecated in [RFC5389]
#define CHANGE_ADDRESS      0x0005 // Deprecated in [RFC5389]
#define USERNAME            0x0006
#define PASSWORD            0x0007 // Deprecated in [RFC5389]
#define MESSAGE_INTEGRITY   0x0008
#define ERROR_CODE          0x0009
#define UNKNOWN_ATTRIBUTE   0x000A
#define REFLECTED_FROM      0x000B // Deprecated in [RFC5389]
#define REALM               0x0014
#define NONCE               0x0015
#define XOR_MAPPED_ADDRESS  0x0020
// Comprehension-optional range (0x8000-0xFFFF)
#define SOFTWARE            0x8022
#define ALTERNATE_SERVER    0x8023
#define FINGERPRINT         0x8028
#define SECONDARY_ADDRESS   0x8050


/// <summary>Creates a socket and binds it to the given IP and port.</summary>
/// <param name="port">Local port to bind socket to</param>
/// <param name="localIP">Local IP to bind socket to</param>
/// <returns>Bound socket</returns>
SOCKET GetBoundSocket(const u_short port, const std::string& localIP = "0.0.0.0")
{
    // Create a socket for sending data.
    const SOCKET sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sendSocket == INVALID_SOCKET) {
        BM_ERROR_LOG("failed to create a socket: {:s}", quote(make_winsock_error_code().message()));
        return INVALID_SOCKET;
    }

    // Bind the socket to the local IP address.
    sockaddr_in addrSrc{};
    addrSrc.sin_family = AF_INET;
    if (!inet_pton(AF_INET, localIP.c_str(), &addrSrc.sin_addr)) {
        closesocket(sendSocket);
        return INVALID_SOCKET;
    }
    addrSrc.sin_port = htons(port);
    if (bind(sendSocket, reinterpret_cast<sockaddr*>(&addrSrc), sizeof addrSrc) == SOCKET_ERROR) {
        BM_ERROR_LOG("failed to bind the socket: {:s}", quote(make_winsock_error_code().message()));
        closesocket(sendSocket);
        return INVALID_SOCKET;
    }

    // Set the sockopt SO_REUSEADDR to reuse the same address.
    BOOL bOptVal = TRUE;
    const int bOptLen = sizeof BOOL;
    if (setsockopt(sendSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&bOptVal), bOptLen) == SOCKET_ERROR) {
        BM_ERROR_LOG("failed set SO_REUSEADDR: {:s}", quote(make_winsock_error_code().message()));
        closesocket(sendSocket);
        return INVALID_SOCKET;
    }

    return sendSocket;
}


void FillBuffer(char* buf, const size_t bufLen, size_t& offset, void* src, const size_t srcSize)
{
    if (offset + srcSize >= bufLen) {
        BM_ERROR_LOG("{:d} >= {:d}", offset + srcSize, bufLen);
        return;
    }

    for (size_t i = srcSize; i > 0; i--) {
        buf[offset++] = static_cast<char*>(src)[i - 1];
    }
}


std::vector<char> GenerateRandomTransactionId(const size_t transIdSize)
{
    std::random_device randomDevice;
    std::vector<char> transactionId(transIdSize);
    size_t i = 0;
    while (i < transIdSize) {
        unsigned int num = randomDevice();
        for (size_t j = 0; j < sizeof num && i < transIdSize; j++) {
            transactionId[i] = num & 0xff;
            num = num >> 8;
            i++;
        }
    }

    return transactionId;
}


/// <summary>Constructs a STUN bind request message.</summary>
/// <param name="buf">Buffer to store the message in</param>
/// <param name="bufLen">Length of the buffer</param>
/// <param name="attrType">Type of attributes to send</param>
/// <param name="changeIP">Change IP address on response</param>
/// <param name="changePort">Change port on response</param>
void ConstructStunRequest(char* buf, size_t* bufLen, u_short attrType, const bool changeIP, const bool changePort)
{
    if (buf == nullptr || bufLen == nullptr) {
        buf = nullptr;
        return;
    }

    size_t offset = 0;
    size_t transIdSize = 16;
    uint16_t lenData;
    uint32_t attrVal;
    uint16_t attrLen;
    switch (attrType) {
        case CHANGE_REQUEST:
            if (changeIP && changePort) {
                attrVal = 0x00000006;
            }
            else if (changeIP) {
                attrVal = 0x00000004;
            }
            else if (changePort) {
                attrVal = 0x00000002;
            }
            else {
                attrVal = 0x00000000;
            }
            attrLen = sizeof attrVal;
            lenData = sizeof attrType + sizeof attrLen + sizeof attrVal;
            break;
        case RESPONSE_ADDRESS:
        default:
            lenData = 0;
            break;
    }

    if (*bufLen <= lenData) {
        buf = nullptr;
        return;
    }

    /* Write STUN message header. */
    // STUN message type.
    uint16_t msgType = BIND_REQUEST_MSG;
    FillBuffer(buf, *bufLen, offset, &msgType, sizeof msgType);
    // STUN message length.
    FillBuffer(buf, *bufLen, offset, &lenData, sizeof lenData);
    if (attrType == RESPONSE_ADDRESS) {
        // STUN message magic cookie.
        uint32_t cookie = MAGIC_COOKIE;
        FillBuffer(buf, *bufLen, offset, &cookie, sizeof cookie);
        transIdSize -= sizeof cookie;
    }
    // STUN message transaction id.
    std::vector<char> transId = GenerateRandomTransactionId(transIdSize);
    FillBuffer(buf, *bufLen, offset, transId.data(), transIdSize);

    /* Write STUN message attributes. */
    if (lenData > 0) {
        // STUN message attributes type.
        FillBuffer(buf, *bufLen, offset, &attrType, sizeof attrType);
        // STUN message attributes length.
        FillBuffer(buf, *bufLen, offset, &attrLen, sizeof attrLen);
        // STUN message attributes value.
        FillBuffer(buf, *bufLen, offset, &attrVal, sizeof attrVal);
    }

    *bufLen = 20 + static_cast<size_t>(lenData);
}


/// <summary>Struct for STUN response.</summary>
struct StunResponse
{
    struct Attr
    {
        uint16_t AttrType = 0;
        uint16_t AttrLen = 0;
        uint16_t Family = 0;
        uint16_t Port = 0;
        std::string IP;
    };

    uint16_t MsgType = 0;
    uint16_t MsgLen = 0;
    char Cookie[4] = "";
    char TransId[12] = "";
    Attr Addr;
};


/// <summary>Parses a STUN response into a <see cref="stunResponse"/>.</summary>
/// <param name="buf">Buffer with the STUN response</param>
/// <param name="bufLen">Length of the buffer</param>
/// <returns>Parsed STUN response</returns>
StunResponse ParseStunResponse(char* buf, int bufLen)
{
    StunResponse resp;
    if (bufLen < 20) {
        BM_ERROR_LOG("got an empty stun response");
        return resp;
    }

    resp.MsgType = static_cast<uint16_t>(buf[0] << 8 | buf[1]);
    if (!IS_SUCCESS_RESP(resp.MsgType)) {
        BM_ERROR_LOG("got an unsuccessful stun response: {:#X}", resp.MsgType);
        return resp;
    }

    resp.MsgLen = static_cast<uint16_t>(buf[2] << 8 | buf[3]);
    if (resp.MsgLen < 12 || resp.MsgLen + 20 > bufLen) {
        BM_ERROR_LOG("got an invalid response message length");
        return resp;
    }

    if (memcpy_s(resp.Cookie, 4, buf + 4, 4) != 0) {
        BM_ERROR_LOG("got an invalid response cookie");
        return resp;
    }
    if (memcpy_s(resp.TransId, 12, buf + 8, 12) != 0) {
        BM_ERROR_LOG("got an invalid response transaction id");
        return resp;
    }

    int base = 20;
    bufLen = resp.MsgLen;
    while (bufLen > 0) {
        resp.Addr.AttrType = static_cast<uint16_t>(buf[base + 0] << 8 | buf[base + 1]);
        resp.Addr.AttrLen = static_cast<uint16_t>(buf[base + 2] << 8 | buf[base + 3]);
        if (resp.Addr.AttrLen == 0) {
            break;
        }
        if (resp.Addr.AttrLen >= 8) {
            if (resp.Addr.AttrType == MAPPED_ADDRESS) {
                resp.Addr.Family = static_cast<uint8_t>(buf[base + 5]);
                if (resp.Addr.Family == IPV4) {
                    resp.Addr.Port = htons(*reinterpret_cast<uint16_t*>(buf + base + 6));
                    resp.Addr.IP = Networking::IPv4ToString(buf + base + 8);
                    break;
                }
            }
            else if (resp.Addr.AttrType == XOR_MAPPED_ADDRESS) {
                resp.Addr.Family = static_cast<uint8_t>(buf[base + 5]);
                if (resp.Addr.Family == IPV4) {
                    resp.Addr.Port = htons(*reinterpret_cast<uint16_t*>(buf + base + 6)) ^ MAGIC_COOKIE_END;
                    const uint32_t ip = ntohl(htonl(*reinterpret_cast<uint32_t*>(buf + base + 8)) ^ MAGIC_COOKIE);
                    resp.Addr.IP = Networking::IPv4ToString(&ip);
                    break;
                }
            }
        }

        base += 4 + resp.Addr.AttrLen;
        bufLen -= 4 + resp.Addr.AttrLen;
    }

    return resp;
}


/// <summary>Send a STUN request to the given address with the given socket.</summary>
/// <param name="sock">Socket to send the request with</param>
/// <param name="addrDest">Address to send the request to</param>
/// <param name="attrType">Type of STUN request to send</param>
/// <param name="changeIP">Whether the STUN server can change its return IP</param>
/// <param name="changePort">Whether the STUN server can change its return port</param>
/// <param name="resp"><see cref="stunResponse"/> to store the response in</param>
/// <returns>Whether the request errored</returns>
int SendStunRequest(const SOCKET sock, sockaddr addrDest, const u_short attrType, const bool changeIP,
    const bool changePort, StunResponse* resp)
{
    // Send a datagram to the receiver up to three times.
    int iResult = 0;
    char sendBuf[1024];
    size_t sendBufLen = sizeof sendBuf;
    ConstructStunRequest(sendBuf, &sendBufLen, attrType, changeIP, changePort);
    std::string strBuf;
    for (size_t i = 0; i < sendBufLen; i++) {
        strBuf += " " + to_hex(sendBuf[i]);
    }
    BM_TRACE_LOG("sending buf[{:d}]: {:s}", sendBufLen, quote(strBuf));
    for (int i = 0; i < 3; i++) {
        if (sendto(sock, sendBuf, static_cast<int>(sendBufLen), 0, &addrDest, sizeof addrDest) == SOCKET_ERROR) {
            BM_ERROR_LOG("failed to send data: {:s}", quote(make_winsock_error_code().message()));
            return SOCKET_ERROR;
        }

        // Wait until data received or timeout.
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        iResult = select(NULL, &fds, nullptr, nullptr, &NETWORK_TIMEOUT);
        if (iResult == SOCKET_ERROR) {
            BM_ERROR_LOG("failed to get the socket status: {:s}", quote(make_winsock_error_code().message()));
            return SOCKET_ERROR;
        }
        if (iResult != 0) {
            break;
        }
        BM_WARNING_LOG("stun request timed out");
    }
    // Connection timed out.
    if (iResult == 0) {
        BM_ERROR_LOG("stun request timed out");
        return SOCKET_ERROR;
    }

    // Set up the addrRetDest structure for the IP address and port from the receiver.
    sockaddr addrRetDest{};
    int addrRetDestSize = sizeof addrRetDest;
    // Receive a datagram from the receiver.
    char recvBuf[1024];
    constexpr size_t recvBufLen = sizeof recvBuf;
    if (recvfrom(sock, recvBuf, static_cast<int>(recvBufLen), NULL, &addrRetDest, &addrRetDestSize) == SOCKET_ERROR) {
        BM_ERROR_LOG("failed to receive data: {:s}", quote(make_winsock_error_code().message()));
        return SOCKET_ERROR;
    }

    *resp = ParseStunResponse(recvBuf, recvBufLen);
    BM_TRACE_LOG(
        "recieved response: {{ MsgType: {:X}, MsgLen: {:d}, Cookie: {:s}, TransId: {:s}, Addr: {:s} }}", resp->MsgType,
        resp->MsgLen, to_hex(resp->Cookie, 4), to_hex(resp->TransId, 12),
        quote(resp->Addr.IP + ":" + std::to_string(resp->Addr.Port)));

    return 0;
}


/// <summary>Reads STUN server addresses from a given file.</summary>
/// <param name="path">File with STUN server addresses</param>
/// <returns>Vector of STUN server addresses in <see cref="sockaddr_in"/></returns>
std::vector<sockaddr> ParseStunServers(const std::filesystem::path& path)
{
    std::vector<sockaddr> stunServers;

    std::ifstream file(path);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            const size_t offset = line.find(':');
            if (offset == std::string::npos) {
                continue;
            }
            std::string ip = line.substr(0, offset);
            std::string port = line.substr(offset + 1);

            // Set up the addrDest structure with the IP address and port of the receiver.
            addrinfo hints{};
            addrinfo* result = nullptr;
            ZeroMemory(&hints, sizeof hints);
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_protocol = IPPROTO_UDP;
            if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &result) != 0) {
                BM_ERROR_LOG("failed to translate {:s}, {:s}", quote(ip + ":" + port),
                    quote(make_winsock_error_code().message()));
                continue;
            }

            stunServers.push_back(*result->ai_addr);
        }
    }

    return stunServers;
}


std::string FormatAddr(sockaddr* addr)
{
    const sockaddr_in* addrIn = reinterpret_cast<sockaddr_in*>(addr);

    return Networking::IPv4ToString(&addrIn->sin_addr) + ":" + std::to_string(ntohs(addrIn->sin_port));
}


/// <summary>Tries to find the type of NAT the network uses.</summary>
/// <remarks>Inspired by https://tools.ietf.org/html/rfc3489</remarks>
/// <param name="port">Port to send the STUN requests through</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void P2PHost::FindNATType(unsigned short port, bool threaded)
{
    if (threaded) {
        natType = NATType::NAT_SEARCHING;
        discoverThread->addJob([this, port]() {
            FindNATType(port, false);
        });
        return;
    }

    // Initialize Winsock.
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        natType = NATType::NAT_ERROR;
        lastError = make_winsock_error_code();
        BM_ERROR_LOG("failed to initiate winsock: {:s}", quote(lastError.message()));
        return;
    }

    // Create a socket for sending data.
    SOCKET sendSocket = GetBoundSocket(port);
    if (sendSocket == INVALID_SOCKET) {
        natType = NATType::NAT_ERROR;
        lastError = make_winsock_error_code();
        WSACleanup();
        return;
    }

    // Send first STUN request.
    sockaddr server{};
    StunResponse resp1;
    bool responded = false;
    std::string stunServerAddr;
    for (sockaddr stunServer : ParseStunServers(STUN_SERVICES_FILE_PATH)) {
        stunServerAddr = FormatAddr(&stunServer);
        BM_TRACE_LOG("sending first stun request to {:s}", quote(stunServerAddr));
        if (SendStunRequest(sendSocket, stunServer, RESPONSE_ADDRESS, false, false, &resp1) != SOCKET_ERROR) {
            server = stunServer;
            responded = true;
            break;
        }
        natType = NATType::NAT_ERROR;
        lastError = make_winsock_error_code();
        BM_ERROR_LOG(
            "failed to connect to {:s}: {:s}", quote(stunServerAddr),
            quote(lastError ? lastError.message() : "Connection timed out."));
    }
    if (!responded) {
        natType = NATType::NAT_BLOCKED;
        closesocket(sendSocket);
        WSACleanup();
        return;
    }

    if (resp1.Addr.Port != port) {
        natType = NATType::NAT_SYMMETRIC;
        closesocket(sendSocket);
        WSACleanup();
        return;
    }

    // Send second STUN request.
    StunResponse resp2;
    BM_TRACE_LOG("sending second stun request to {:s} which IP change request", quote(stunServerAddr));
    if (SendStunRequest(sendSocket, server, CHANGE_REQUEST, true, true, &resp1) != SOCKET_ERROR) {
        natType = NATType::NAT_FULL_CONE;
        closesocket(sendSocket);
        WSACleanup();
        return;
    }

    // Send third STUN request.
    StunResponse resp3;
    BM_TRACE_LOG("sending third stun request to {:s}", quote(stunServerAddr));
    if (SendStunRequest(sendSocket, server, RESPONSE_ADDRESS, false, false, &resp1) == SOCKET_ERROR) {
        natType = NATType::NAT_ERROR;
        lastError = make_winsock_error_code();
        closesocket(sendSocket);
        WSACleanup();
        return;
    }

    if (resp1.Addr.IP == resp3.Addr.IP && resp1.Addr.Port == resp3.Addr.Port) {
        // Send fourth STUN request.
        StunResponse resp4;
        BM_TRACE_LOG("sending fourth stun request to {:s} which port change request", quote(stunServerAddr));
        if (SendStunRequest(sendSocket, server, CHANGE_REQUEST, false, true, &resp1) == SOCKET_ERROR) {
            natType = NATType::NAT_RESTRICTED;
        }
        else {
            natType = NATType::NAT_RESTRICTED_PORT;
        }
    }
    else {
        natType = NATType::NAT_SYMMETRIC;
    }

    closesocket(sendSocket);
    WSACleanup();
}


/// <summary>Tries to punch a hole in the NAT for a specific user.</summary>
/// <param name="ip">IP address to punch for</param>
/// <param name="port">Port to punch</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void P2PHost::PunchPort(const std::string& ip, unsigned short port, const bool threaded)
{
    if (threaded) {
        discoverThread->addJob([this, ip, port]() {
            PunchPort(ip, port, false);
        });
        return;
    }

    // Initialize WinSock.
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        BM_ERROR_LOG("failed to initiate winsock: {:s}", quote(make_winsock_error_code().message()));
        return;
    }

    // Create a socket for sending data.
    const SOCKET sendSocket = GetBoundSocket(port);
    if (sendSocket == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    // Set up the addrDest structure with the IP address and port of the receiver.
    sockaddr_in addrDest{};
    addrDest.sin_family = AF_INET;
    addrDest.sin_port = htons(port);
    if (inet_pton(addrDest.sin_family, ip.c_str(), &addrDest.sin_addr.s_addr) != 1) {
        closesocket(sendSocket);
        WSACleanup();
        return;
    }

    // Send a datagram to the receiver
    char sendBuf = '\0';
    constexpr size_t sendBufLen = sizeof sendBuf;
    sendto(
        sendSocket, &sendBuf, static_cast<int>(sendBufLen), 0, reinterpret_cast<sockaddr*>(&addrDest), sizeof addrDest);

    closesocket(sendSocket);
    WSACleanup();
}


/// <summary>Gets the NAT type description.</summary>
/// <returns>A description of the NAT type</returns>
std::string P2PHost::GetNATDesc() const
{
    switch (natType) {
        case NATType::NAT_NONE:
            return "";
        case NATType::NAT_SEARCHING:
            return "Connecting with STUN server.";
        case NATType::NAT_BLOCKED:
            return "None of the STUN servers responded.\nMake sure you have access to the internet.";
        case NATType::NAT_FULL_CONE:
            return "Found a full-cone NAT, this means that your router will let any connection in as long as you "
                "send a request through that port.\nTo let others establish a connection with your server, you need"
                "to send a random message through your desired port.";
        case NATType::NAT_RESTRICTED:
            return "Found a restricted NAT, there is currently no way to initiate a peer to peer connection.";
        case NATType::NAT_RESTRICTED_PORT:
            return "Found a port restricted NAT, this means that your router will let any connection from the same "
                "IP in as long as your send a request through that port.\nTo let others establish a connection with "
                "your server, you need to send a message to the client that are joining you through your desired port.";
        case NATType::NAT_SYMMETRIC:
            return "Found a symmetric NAT, there is currently no way to initiate a peer to peer connection.";
        case NATType::NAT_ERROR:
            if (!lastError) {
                return "Connection timed-out, retrying.";
            }
            return "An error network error occurred: " + lastError.message() + " (" + to_hex(lastError.value()) + ")";
    }

    return "Unknown status";
}


/// <summary>Initializes the discovery thread <see cref="WorkerThread"/>.</summary>
P2PHost::P2PHost()
{
    discoverThread = std::make_unique<JobQueue>();
}
