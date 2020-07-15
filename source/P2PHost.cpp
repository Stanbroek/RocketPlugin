// P2PHost.cpp
// UDP hole punching using STUN for the RocketPlugin plugin.
//
// Author:       Stanbroek
// Version:      0.6.3 15/7/20
//
// References:
//  https://en.wikipedia.org/wiki/UDP_hole_punching
//  https://www.ietf.org/rfc/rfc3489.txt
//  https://www.ietf.org/rfc/rfc5389.txt

#include "Networking.h"

#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <filesystem>
#include <fstream>
#include <map>

/* Determining STUN message types */
#define IS_REQUEST(msg_type)      (((msg_type) & 0x0110) == 0x0000)
#define IS_INDICATION(msg_type)   (((msg_type) & 0x0110) == 0x0010)
#define IS_SUCCESS_RESP(msg_type) (((msg_type) & 0x0110) == 0x0100)
#define IS_ERR_RESP(msg_type)     (((msg_type) & 0x0110) == 0x0110)

/* Magic cookie */
#define MAGIC_COOKIE  0x2112A442
#define MAGIC_COOKIE_ (MAGIC_COOKIE >> 16)
#define _MAGIC_COOKIE (MAGIC_COOKIE & 0xffff)

/* Address family */
#define IPV4 0x01
#define IPV6 0x02

/* STUN message types */
#define BINDREQUESTMSG               0x0001
#define BINDRESPONSEMSG              0x0101
#define BINDERRORRESPONSEMSG         0x0111
#define SHAREDSECRETREQUESTMSG       0x0002
#define SHAREDSECRETRESPONSEMSG      0x0102
#define SHAREDSECRETERRORRESPONSEMSG 0x0112

/* STUN attributes types */
// Comprehension-required range (0x0000-0x7FFF):
#define MAPPED_ADDRESS     0x0001
#define RESPONSE_ADDRESS   0x0002 // Deprecated in [RFC5389]
#define CHANGE_REQUEST     0x0003 // Deprecated in [RFC5389]
#define SOURCE_ADDRESS     0x0004 // Deprecated in [RFC5389]
#define CHANGE_ADDRESS     0x0005 // Deprecated in [RFC5389]
#define USERNAME           0x0006
#define PASSWORD           0x0007 // Deprecated in [RFC5389]
#define MESSAGE_INTEGRITY  0x0008
#define ERROR_CODE         0x0009
#define UNKNOWN_ATTRIBUTE  0x000A
#define REFLECTED_FROM     0x000B // Deprecated in [RFC5389]
#define REALM              0x0014
#define NONCE              0x0015
#define XOR_MAPPED_ADDRESS 0x0020
// Comprehension-optional range (0x8000-0xFFFF)
#define SOFTWARE           0x8022
#define ALTERNATE_SERVER   0x8023
#define FINGERPRINT        0x8028
#define SECONDARYADDRESS   0x8050


/// <summary>Creates a socket and binds it to the given IP and port.</summary>
/// <param name="port">Local port to bind socket to</param>
/// <param name="localIP">Local IP to bind socket to</param>
/// <returns>Bound socket</returns>
SOCKET getBoundSocket(u_short port, std::string localIP = "0.0.0.0")
{
    // Create a socket for sending data.
    SOCKET sendSocket = INVALID_SOCKET;
    sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sendSocket == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }

    // Bind the socket to the local IP address.
    sockaddr_in addrSrc;
    addrSrc.sin_family = AF_INET;
    if (!inet_pton(AF_INET, localIP.c_str(), &addrSrc.sin_addr)) {
        closesocket(sendSocket);
        return INVALID_SOCKET;
    }
    addrSrc.sin_port = htons(port);
    if (bind(sendSocket, (sockaddr*)&addrSrc, sizeof addrSrc) == SOCKET_ERROR) {
        closesocket(sendSocket);
        return INVALID_SOCKET;
    }

    // Set the sockopt SO_REUSEADDR to reuse the same address.
    BOOL bOptVal = TRUE;
    int bOptLen = sizeof(BOOL);
    if (setsockopt(sendSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&bOptVal, bOptLen) == SOCKET_ERROR) {
        closesocket(sendSocket);
        return INVALID_SOCKET;
    }

    return sendSocket;
}


/// <summary>Constructs a STUN bind request message.</summary>
/// <param name="buf">Buffer to store the message in</param>
/// <param name="bufLen">Length of the buffer</param>
/// <param name="attrType">Type of attributes to send</param>
/// <param name="changeIP">Change IP address on response</param>
/// <param name="changePort">Change port on response</param>
void constructStunRequest(char* buf, int* bufLen, u_short attrType, bool changeIP, bool changePort)
{
    if (buf == nullptr || bufLen == nullptr) {
        buf = nullptr;
        return;
    }

    int offset = 0;
    int transIdSize = 16;
    uint16_t lenData;
    uint32_t attrVal;
    uint16_t attrLen;
    switch (attrType)
    {
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
        transIdSize = 12;
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
    uint16_t msgType = BINDREQUESTMSG;
    for (int i = sizeof msgType - 1; i >= 0; i--) {
        buf[offset++] = ((char*)&msgType)[i];
    }
    // STUN message length.
    for (int i = sizeof lenData - 1; i >= 0; i--) {
        buf[offset++] = ((char*)&lenData)[i];
    }
    if (attrType == RESPONSE_ADDRESS) {
        // STUN message magic cookie.
        uint32_t cookie = MAGIC_COOKIE;
        for (int i = sizeof cookie - 1; i >= 0; i--) {
            buf[offset++] = ((char*)&cookie)[i];
        }
    }
    // STUN message transaction id.
    for (int i = 0; i < transIdSize; i++) {
        buf[offset++] = rand() % UCHAR_MAX;
    }

    /* Write STUN message attributes. */
    if (lenData > 0) {
        // STUN message attributes type.
        for (int i = sizeof attrType - 1; i >= 0; i--) {
            buf[offset++] = ((char*)&attrType)[i];
        }
        // STUN message attributes length.
        for (int i = sizeof attrLen - 1; i >= 0; i--) {
            buf[offset++] = ((char*)&attrLen)[i];
        }
        // STUN message attributes value.
        for (int i = sizeof attrVal - 1; i >= 0; i--) {
            buf[offset++] = ((char*)&attrVal)[i];
        }
    }

    *bufLen = 20 + lenData;
}


/// <summary>Struct for STUN response.</summary>
struct stunResponse {
    struct attr {
        uint16_t attrType = 0;
        uint16_t attrLen = 0;
        uint16_t family = 0;
        uint16_t port = 0;
        std::string ip = "";
    };

    uint16_t msgType = 0;
    uint16_t msgLen = 0;
    char coockie[4] = "";
    char transid[12] = "";
    attr addr;
};


/// <summary>Parses a STUN response into a <see cref="stunResponse"/>.</summary>
/// <param name="buf">Buffer with the STUN response</param>
/// <param name="bufLen">Length of the buffer</param>
/// <returns>Parsed STUN response</returns>
stunResponse parseStunResponse(char* buf, int bufLen)
{
    stunResponse resp;
    if (bufLen < 20) {
        return resp;
    }

    resp.msgType = buf[0] << 8 | buf[1];
    if (!IS_SUCCESS_RESP(resp.msgType)) {
        return resp;
    }

    resp.msgLen = buf[2] << 8 | buf[3];
    if (resp.msgLen < 12 || resp.msgLen + 20 > bufLen) {
        return resp;
    }

    if (memcpy_s(resp.coockie, 4, buf + 4, 4) != 0) {
        return resp;
    }
    if (memcpy_s(resp.transid, 12, buf + 8, 12) != 0) {
        return resp;
    }

    int base = 20;
    bufLen = resp.msgLen;
    while (bufLen > 0) {
        resp.addr.attrType = buf[base + 0] << 8 | buf[base + 1];
        resp.addr.attrLen = buf[base + 2] << 8 | buf[base + 3];
        if (resp.addr.attrLen == 0) {
            break;
        }
        else if (resp.addr.attrLen >= 8) {
            if (resp.addr.attrType == MAPPED_ADDRESS) {
                resp.addr.family = buf[base + 5];
                if (resp.addr.family == IPV4) {
                    resp.addr.port = htons(*(uint16_t*)(buf + base + 6));
                    char ipBuf[sizeof "255.255.255.255"];
                    resp.addr.ip = inet_ntop(AF_INET, buf + base + 8, ipBuf, sizeof ipBuf);
                    break;
                }
            }
            else if (resp.addr.attrType == XOR_MAPPED_ADDRESS) {
                resp.addr.family = buf[base + 5];
                if (resp.addr.family == IPV4) {
                    resp.addr.port = htons(*(uint16_t*)(buf + base + 6)) ^ MAGIC_COOKIE_;
                    char ipBuf[sizeof "255.255.255.255"];
                    uint32_t ip = ntohl(htonl(*(uint32_t*)(buf + base + 8)) ^ MAGIC_COOKIE);
                    resp.addr.ip = inet_ntop(AF_INET, &ip, ipBuf, sizeof ipBuf);
                    break;
                }
            }
        }

        base += 4 + resp.addr.attrLen;
        bufLen -= 4 + resp.addr.attrLen;
    }

    return resp;
}


/// <summary>Send a STUN request to the given address with the given socket.</summary>
/// <param name="sock">Socket to send the request with</param>
/// <param name="address">Address to send the request to</param>
/// <param name="port">Port to send the request to</param>
/// <param name="attrType">Type of attributes to send</param>
/// <param name="resp"><see cref="stunResponse"/> to store the response in</param>
/// <returns>Whether the request errored</returns>
int sendStunRequest(SOCKET sock, sockaddr_in addrDest, u_short attrType, bool changeIP, bool changePort, stunResponse* resp)
{
    // Send a datagram to the receiver up to three times.
    int iResult = 0;
    char sendBuf[1024];
    int sendBufLen = sizeof sendBuf;
    constructStunRequest(sendBuf, &sendBufLen, attrType, changeIP, changePort);
    for (int i = 0; i < 3; i++) {
        if (sendto(sock, sendBuf, sendBufLen, 0, (sockaddr*)&addrDest, sizeof addrDest) == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }

        // Wait until data received or timeout.
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        struct timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        iResult = select(NULL, &fds, NULL, NULL, &tv);
        if (iResult == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
        // Connection timed out.
        else if (iResult != 0) {
            break;
        }
    }
    if (iResult == 0) {
        return SOCKET_ERROR;
    }

    // Set up the addrRetDest structure for the IP address and port from the receiver.
    sockaddr_in addrRetDest;
    int addrRetDestSize = sizeof addrRetDest;
    // Recieve a datagram from the receiver.
    char recvBuf[1024];
    int recvBufLen = 1024;
    if (recvfrom(sock, recvBuf, recvBufLen, 0, (sockaddr*)&addrRetDest, &addrRetDestSize) == SOCKET_ERROR) {
        return SOCKET_ERROR;
    }
    
    *resp = parseStunResponse(recvBuf, recvBufLen);

    return 0;
}


/// <summary>Reads STUN server addresses from a given file.</summary>
/// <param name="path">File with STUN server addresses</param>
/// <returns>Vector of STUN server addresses in <see cref="sockaddr_in"/></returns>
std::vector<sockaddr_in> parseSTUNServers(std::string path)
{
    std::vector<sockaddr_in> stunServers;
    std::ifstream file(path);

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            size_t delim = line.find(':');
            if (delim == std::string::npos) {
                continue;
            }
            std::string ip = line.substr(0, delim);
            std::string port = line.substr(delim + 1);

            // Set up the addrDest structure with the IP address and port of the receiver.
            addrinfo hints;
            addrinfo* result = NULL;
            ZeroMemory(&hints, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_protocol = IPPROTO_UDP;
            if (getaddrinfo(ip.c_str(), NULL, &hints, &result) != 0) {
                continue;
            }
            sockaddr_in address;
            address = *(sockaddr_in*)result->ai_addr;
            address.sin_family = AF_INET;
            address.sin_port = htons((u_short)std::atoi(port.c_str()));

            stunServers.push_back(address);
        }
    }


    return stunServers;
}


/// <summary>Tries to find the type of NAT the network uses.</summary>
/// <remarks>Inspired by https://tools.ietf.org/html/rfc3489</remarks>
/// <param name="port">Port to send the STUN requests through</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void P2PHost::findNATType(unsigned short port, bool threaded)
{
    if (threaded) {
        natType = NATType::NAT_SEARCHING;
        discoverThread->addJob([this, port]() { findNATType(port, false); });
        return;
    }

    // Initialize Winsock.
    WSADATA wsaData = { 0 };
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        natType = NATType::NAT_ERROR;
        return;
    }

    // Create a socket for sending data.
    SOCKET sendSocket = getBoundSocket(port);
    if (sendSocket == INVALID_SOCKET) {
        natType = NATType::NAT_ERROR;
        WSACleanup();
        return;
    }

    // Send first STUN request.
    stunResponse resp1;
    sockaddr_in server = { 0 };
    bool responded = false;
    std::vector<sockaddr_in> stunServers = parseSTUNServers("./bakkesmod/data/rocketplugin/STUN-services.txt");
    for (auto stunServer : stunServers) {
        if (sendStunRequest(sendSocket, stunServer, RESPONSE_ADDRESS, false, false, &resp1) != SOCKET_ERROR) {
            server = stunServer;
            responded = true;
            break;
        }
        else {
            natType = NATType::NAT_ERROR;
        }
    }
    if (!responded) {
        natType = NATType::NAT_BLOCKED;
        closesocket(sendSocket);
        WSACleanup();
        return;
    }

    // Send second STUN request.
    stunResponse resp2;
    if (sendStunRequest(sendSocket, server, CHANGE_REQUEST, true, true, &resp1) != SOCKET_ERROR) {
        natType = NATType::NAT_FULL_CONE;
        closesocket(sendSocket);
        WSACleanup();
        return;
    }

    // Send third STUN request.
    stunResponse resp3;
    if (sendStunRequest(sendSocket, server, RESPONSE_ADDRESS, false, false, &resp1) == SOCKET_ERROR) {
        natType = NATType::NAT_ERROR;
        closesocket(sendSocket);
        WSACleanup();
        return;
    }

    if (resp1.addr.ip == resp3.addr.ip && resp1.addr.port == resp3.addr.port) {
        // Send fourth STUN request.
        stunResponse resp4;
        if (sendStunRequest(sendSocket, server, CHANGE_REQUEST, false, true, &resp1) == SOCKET_ERROR) {
            natType = NATType::NAT_RESTRIC;
        }
        else {
            natType = NATType::NAT_RESTRIC_PORT;
        }
    }
    else {
        natType = NATType::NAT_SYMMETRIC;
    }

    closesocket(sendSocket);
    WSACleanup();
}


/// <summary>Tries to punch a hole in the NAT for a specific user.</summary>
/// <param name="IP">IP address to punch for</param>
/// <param name="port">Port to punch</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void P2PHost::punchPort(std::string IP, unsigned short port, bool threaded)
{
    if (threaded) {
        discoverThread->addJob([this, IP, port]() { punchPort(IP, port, false); });
        return;
    }

    // Initialize Winsock.
    WSADATA wsaData = { 0 };
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        return;
    }

    // Create a socket for sending data.
    SOCKET sendSocket = getBoundSocket(port);
    if (sendSocket == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    // Set up the addrDest structure with the IP address and port of the receiver.
    sockaddr_in addrDest;
    addrDest.sin_family = AF_INET;
    addrDest.sin_port = htons(port);
    if (inet_pton(addrDest.sin_family, IP.c_str(), &addrDest.sin_addr.s_addr) != 1) {
        closesocket(sendSocket);
        WSACleanup();
        return;
    }

    // Send a datagram to the receiver
    int sendBufLen = 0;
    char sendBuf = '\0';
    sendto(sendSocket, &sendBuf, sendBufLen, 0, (sockaddr*)&addrDest, sizeof addrDest);

    closesocket(sendSocket);
    WSACleanup();
}


/// <summary>Gets the NAT type description.</summary>
/// <returns>A description of the NAT type</returns>
std::string P2PHost::getNATDesc()
{
    int lastError = WSAGetLastError();

    switch (natType)
    {
    case NATType::NAT_YET_DETERMINED:
        return "";
        break;
    case NATType::NAT_SEARCHING:
        return "Connecting with STUN server.";
        break;
    case NATType::NAT_BLOCKED:
        return "Non of the STUN servers responded.\nMake sure you have access to the internet.";
        break;
    case NATType::NAT_FULL_CONE:
        return "Found a full-cone NAT, this means that your router will let any connection in as long as you send a request through that port.\nTo let others establish a connection with your server, you need to send a random message through your desired port.";
        break;
    case NATType::NAT_RESTRIC:
        return "Found a restricted NAT, there is currently no way to initiate a peer to peer connection.";
        break;
    case NATType::NAT_RESTRIC_PORT:
        return "Found a port restricted NAT, this means that your router will let any connection from the same IP in as long as your send a request through that port.\nTo let others establish a connection with your server, you need to send a message to the client that are joining you through your desired port.";
        break;
    case NATType::NAT_SYMMETRIC:
        return "Found a symmetric NAT, there is currently no way to initiate a peer to peer connection.";
        break;
    case NATType::NAT_ERROR:
        if (lastError == 0) {
            return "Connection timed-out, retrying.";
        }
        return "An error network error occurred: " + std::system_category().message(lastError) + " (" + std::to_string(lastError) + ")";
        break;
    default:
        break;
    }

    return "Unknown status";
}


/// <summary>Gets the NAT type.</summary>
/// <returns>The NAT type</returns>
P2PHost::NATType P2PHost::getNATType()
{
    return natType;
}


/// <summary>Initializes the discovery thread <see cref="WorkerThread"/>.</summary>
P2PHost::P2PHost()
{
    discoverThread = std::unique_ptr<WorkerThread>(new WorkerThread());
}