// Networking.cpp
// General networking calls for the RocketPlugin plugin.
//
// Author:       Stanbroek
// Version:      0.6.5 05/02/21

#include "Networking.h"

#include <regex>
#include <system_error>

#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

#include "utils/win32_error_category.h"

constexpr timeval NETWORK_TIMEOUT = { 3, 0 };


/// <summary>Get the type of address that is given.</summary>
/// <param name="addr">address to get the type of</param>
/// <returns>The type of the given address</returns>
Networking::DestAddrType Networking::GetDestAddrType(const char* addr)
{
    if (IsValidIPv4(addr)) {
        if (IsPrivateIPv4(addr)) {
            return DestAddrType::PRIVATE_ADDR;
        }
        if (IsHamachiIPv4(addr)) {
            return DestAddrType::HAMACHI_ADDR;
        }
        if (IsExternalIPv4(addr)) {
            return DestAddrType::EXTERNL_ADDR;
        }
        return DestAddrType::INTERNL_ADDR;
    }
    if (IsValidDomainName(addr)) {
        return DestAddrType::EXTERNL_ADDR;
    }

    return DestAddrType::UNKNOWN_ADDR;
}


/// <summary>Gets a hint for the given address type and host status.</summary>
/// <param name="addrType">Address type to get the hint for</param>
/// <param name="hostStatus">Host status to get the hint for</param>
/// <returns>Hint for the given address type and host status</returns>
std::string Networking::GetHostStatusHint(const DestAddrType addrType, const HostStatus hostStatus)
{
    std::string hint;

    switch (addrType) {
        case DestAddrType::UNKNOWN_ADDR:
            hint = "Invalid address.";
            break;
        case DestAddrType::PRIVATE_ADDR:
        case DestAddrType::INTERNL_ADDR:
            hint = "Warning, this is not an external IP address.";
            break;
        case DestAddrType::HAMACHI_ADDR:
            hint = "Valid Hamachi address.";
            break;
        case DestAddrType::EXTERNL_ADDR:
            hint = "Valid external address.";
            break;
    }

    switch (hostStatus) {
        case HostStatus::HOST_UNKNOWN:
            break;
        case HostStatus::HOST_BUSY:
            hint += "\nTrying to reach the host.";
            break;
        case HostStatus::HOST_ERROR:
            hint += "\nAn error occurred while trying to reach the host.";
            break;
        case HostStatus::HOST_TIMEOUT:
            hint += "\nTimed out while trying to reach the host.";
            break;
        case HostStatus::HOST_ONLINE:
            hint += "\nSuccessfully reached the host.";
            break;
    }

    return hint;
}


std::string Networking::IPv4ToString(const void* addr)
{
    const unsigned char* ip = static_cast<const unsigned char*>(addr);
    return std::to_string(ip[0]) + "." + std::to_string(ip[1]) + "." + std::to_string(ip[2]) + "." +
        std::to_string(ip[3]);
}


/// <summary>Checks whether the given port number is a valid port.</summary>
/// <param name="port">port to validate</param>
/// <returns>Bool with is the port number is a valid port</returns>
bool Networking::IsValidPort(const int port)
{
    return port > 1023 && port < 0xFFFF;
}


/// <summary>Checks whether the given IP is a valid ipv4.</summary>
/// <param name="ipAddr">IP to validate</param>
/// <returns>Bool with is the IP is a valid ipv4 address</returns>
bool Networking::IsValidIPv4(const std::string& ipAddr)
{
    static const std::regex ipv4Regex(
        "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");
    std::smatch match;

    if (std::regex_match(ipAddr, match, ipv4Regex)) {
        return true;
    }

    return false;
}


/// <summary>Checks whether the given IP is an private ipv4 address.</summary>
/// <param name="ipAddr">IP to validate</param>
/// <returns>Bool with is the IP is an private ipv4 address</returns>
bool Networking::IsPrivateIPv4(const std::string& ipAddr)
{
    if (!IsValidIPv4(ipAddr)) {
        return false;
    }

    // 10.0.0.0     -   10.255.255.255  (10/8 prefix)       - private networks
    if (std::strncmp(ipAddr.c_str(), "10.", 3) == 0) {
        return true;
    }
    // 172.16.0.0   -   172.31.255.255  (172.16/12 prefix)  - private networks
    if (std::strncmp(ipAddr.c_str(), "172.", 4) == 0) {
        const long i = strtol(ipAddr.data() + 4, nullptr, 10);
        if (16 <= i && i <= 31) {
            return true;
        }
    }
    // 192.168.0.0  -   192.168.255.255 (192.168/16 prefix) - private networks
    if (std::strncmp(ipAddr.c_str(), "192.168.", 8) == 0) {
        return true;
    }

    return false;
}


/// <summary>Checks whether the given IP is an external ipv4 address.</summary>
/// <param name="ipAddr">IP to validate</param>
/// <returns>Bool with is the IP is an external ipv4 address</returns>
bool Networking::IsExternalIPv4(const std::string& ipAddr)
{
    if (IsPrivateIPv4(ipAddr)) {
        return false;
    }

    // 100.64.0.0   -   100.127.255.255  (100.64/10 prefix) - carrier-grade NAT deployment
    if (std::strncmp(ipAddr.c_str(), "100.", 4) == 0) {
        const long i = strtol(ipAddr.data() + 4, nullptr, 10);
        if (64 <= i && i <= 127) {
            return false;
        }
    }
    // 127.0.0.0    -   127.255.255.255 (127/8 prefix)      - localhost
    if (std::strncmp(ipAddr.c_str(), "127.", 4) == 0) {
        return false;
    }

    return true;
}


/// <summary>Checks whether the given IP is in the Hamachi ipv4 address space.</summary>
/// <param name="ipAddr">IP to validate</param>
/// <returns>Bool with is the IP is in the Hamachi ipv4 address space</returns>
bool Networking::IsHamachiIPv4(const std::string& ipAddr)
{
    if (!IsValidIPv4(ipAddr)) {
        return false;
    }

    // 25.0.0.0        -   25.255.255.255  (10/8 prefix)
    if (std::strncmp(ipAddr.c_str(), "25.", 3) == 0) {
        return true;
    }

    return false;
}


/// <summary>Checks whether the given address is a valid domain name.</summary>
/// <remarks>From https://stackoverflow.com/a/30007882 </remarks>
/// <param name="addr">address to validate</param>
/// <returns>Bool with is the address is a valid domain name</returns>
bool Networking::IsValidDomainName(const std::string& addr)
{
    static const std::regex domainNameRegex(
        "^(?:[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\\.)+[a-z0-9][a-z0-9-]{0,61}[a-z0-9]$");
    std::smatch match;

    if (std::regex_match(addr, match, domainNameRegex)) {
        return true;
    }

    return false;
}


/// <summary>Send data through a websocket.</summary>
/// <param name="host">Host to send the request to</param>
/// <param name="port">Port to send the request to</param>
/// <param name="protocol">Protocol to send the request over</param>
/// <param name="sendBuf">Send buffer</param>
/// <param name="sendBufSize">Size of the send buffer</param>
/// <param name="recvBuf">Receive buffer</param>
/// <param name="recvBufSize">Size of the receive buffer</param>
/// <returns>Error code</returns>
std::error_code Networking::NetworkRequest(const std::string& host, const unsigned short port, const int protocol,
                                           const char* sendBuf, const size_t sendBufSize, char* recvBuf,
                                           const size_t recvBufSize)
{
    // Initialize WinSock.
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        return make_win32_error_code(iResult);
    }

    // Determine socket type.
    int sockType;
    switch (protocol) {
        case IPPROTO_TCP:
            sockType = SOCK_STREAM;
            break;
        case IPPROTO_UDP:
            sockType = SOCK_DGRAM;
            break;
        default:
            WSACleanup();
            return make_win32_error_code(WSAEINVAL);
    }

    // Set up the addrDest structure with the IP address and port of the receiver.
    addrinfo hints{};
    ZeroMemory(&hints, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = sockType;
    hints.ai_protocol = protocol;
    addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        const std::error_code error = make_winsock_error_code();
        WSACleanup();
        return error;
    }
    sockaddr destAddr = *result->ai_addr;

    // Create a socket for sending data.
    const SOCKET sendSocket = socket(AF_INET, sockType, protocol);
    if (sendSocket == INVALID_SOCKET) {
        const std::error_code error = make_winsock_error_code();
        WSACleanup();
        return error;
    }

    if (protocol == IPPROTO_TCP) {
        if (connect(sendSocket, &destAddr, sizeof destAddr) == SOCKET_ERROR) {
            const std::error_code error = make_winsock_error_code();
            closesocket(sendSocket);
            WSACleanup();
            return error;
        }
    }

    // Send a data to the receiver.
    if (sendto(sendSocket, sendBuf, static_cast<int>(sendBufSize), NULL, &destAddr, sizeof destAddr) == SOCKET_ERROR) {
        const std::error_code error = make_winsock_error_code();
        closesocket(sendSocket);
        WSACleanup();
        return error;
    }

    // Wait until data received or timeout.
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sendSocket, &fds);
    iResult = select(NULL, &fds, nullptr, nullptr, &NETWORK_TIMEOUT);
    if (iResult == SOCKET_ERROR) {
        const std::error_code error = make_winsock_error_code();
        closesocket(sendSocket);
        WSACleanup();
        return error;
    }
    // Connection timed out.
    if (iResult == 0) {
        closesocket(sendSocket);
        WSACleanup();
        return make_win32_error_code(WSAETIMEDOUT);
    }

    if (recvBuf != nullptr) {
        // Set up the addrRetDest structure for the IP address and port from the receiver.
        sockaddr_in addrRetDest{};
        int addrRetDestSize = sizeof addrRetDest;
        // Receive a datagram from the receiver.
        if (recvfrom(sendSocket, recvBuf, static_cast<int>(recvBufSize), NULL,
                     reinterpret_cast<sockaddr*>(&addrRetDest), &addrRetDestSize) == SOCKET_ERROR) {
            const std::error_code error = make_winsock_error_code();
            closesocket(sendSocket);
            WSACleanup();
            return error;
        }
    }

    closesocket(sendSocket);
    WSACleanup();

    return make_winsock_error_code();
}


/// <summary>Gets the internal IPv4 address of the user.</summary>
/// <remarks>From https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getipaddrtable </remarks>
/// <returns>Error code</returns>
std::error_code Networking::GetInternalIPAddress(std::string& ipAddr)
{
    DWORD dwSize = 0;
    DWORD dwRetVal = NO_ERROR;
    PMIB_IPADDRTABLE pIPAddrTable = static_cast<PMIB_IPADDRTABLE>(malloc(sizeof MIB_IPADDRTABLE));
    if (pIPAddrTable != nullptr) {
        // Make an initial call to GetIpAddrTable to get the necessary size into the dwSize variable.
        dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, FALSE);
        free(pIPAddrTable);
        pIPAddrTable = nullptr;
        if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
            pIPAddrTable = static_cast<PMIB_IPADDRTABLE>(malloc(dwSize));
        }
    }
    if (pIPAddrTable != nullptr) {
        // Make a second call to GetIpAddrTable to get the actual data we want.
        dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, 0);
        if (dwRetVal == NO_ERROR) {
            TRACE_LOG("got {} entries", pIPAddrTable->dwNumEntries);
            for (size_t i = 0; i < pIPAddrTable->dwNumEntries; i++) {
                if (pIPAddrTable->table[i].wType & MIB_IPADDR_PRIMARY) {
                    IN_ADDR inAddr;
                    inAddr.s_addr = static_cast<ULONG>(pIPAddrTable->table[i].dwAddr);
                    ipAddr = IPv4ToString(&inAddr);
                    TRACE_LOG("found {}", quote(ipAddr));
                    if (IsPrivateIPv4(ipAddr)) {
                        break;
                    }
                    ipAddr.clear();
                }
            }
        }
        else {
            ERROR_LOG("failed to get the ip addr table");
        }
        free(pIPAddrTable);
        pIPAddrTable = nullptr;
    }
    else {
        ERROR_LOG("failed to alloc the ip addr table");
    }

    return make_win32_error_code(dwRetVal);
}


/// <summary>Tries to parse the external IP address from a http response.</summary>
/// <param name="buffer">Http response</param>
/// <returns>External IP address</returns>
std::string parseExternalIPAddressFromResponse(const std::string& buffer)
{
    const size_t contentLengthOffset = buffer.find("Content-Length:") + 16;
    const size_t contentLengthCount = buffer.find('\n', contentLengthOffset);
    const int contentLength = strtol(buffer.substr(contentLengthOffset, contentLengthCount).c_str(), nullptr, 10);

    const size_t addrOffset = buffer.find("\r\n\r\n");
    if (addrOffset == std::string::npos) {
        return "";
    }

    return buffer.substr(addrOffset + 4, contentLength);
}


/// <summary>Tries to get the external IP address.</summary>
/// <param name="host"></param>
/// <param name="ipAddr">Contains the external IP address when threaded</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
/// <returns>External IP address</returns>
std::error_code Networking::GetExternalIPAddress(const std::string& host, std::string* ipAddr, const bool threaded)
{
    if (threaded) {
        std::thread(GetExternalIPAddress, host, ipAddr, false).detach();
        return make_win32_error_code(NULL);
    }

    std::string sendBuf = "GET / HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
    char recvBuf[1024] = "";
    const std::error_code error = NetworkRequest(host, 80, IPPROTO_TCP, sendBuf.data(), sendBuf.size() + 1, recvBuf,
                                                 sizeof recvBuf);
    if (error) {
        return error;
    }

    *ipAddr = parseExternalIPAddressFromResponse(recvBuf);
    if (!IsExternalIPv4(*ipAddr)) {
        ipAddr->clear();
        return make_win32_error_code(WSAHOST_NOT_FOUND);
    }

    return make_win32_error_code(NULL);
}


/// <summary>Tries ping host to see if they are reachable.</summary>
/// <param name="host">Address of the host</param>
/// <param name="port">Port of the server</param>
/// <param name="result">Optional request status</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
/// <returns>Bool with if the host was reachable</returns>
bool Networking::PingHost(const std::string& host, unsigned short port, HostStatus* result, const bool threaded)
{
    if (threaded && result == nullptr) {
        return false;
    }
    if (threaded) {
        if (result != nullptr) {
            *result = HostStatus::HOST_BUSY;
        }
        std::thread(PingHost, host, port, result, false).detach();
        return false;
    }

    const char sendBuf[] = "Hey host guy are you alive?";
    const std::error_code error = NetworkRequest(host, port, IPPROTO_UDP, sendBuf, sizeof sendBuf);
    if (error) {
        if (error == make_win32_error_code(WSAETIMEDOUT)) {
            if (result != nullptr) {
                *result = HostStatus::HOST_TIMEOUT;
            }
            return false;
        }
        if (result != nullptr) {
            *result = HostStatus::HOST_ERROR;
        }
        return false;
    }
    if (result != nullptr) {
        *result = HostStatus::HOST_ONLINE;
    }

    return true;
}
