// Networking.cpp
// General networking calls for the RocketPlugin plugin.
//
// Author:       Stanbroek
// Version:      0.6.3 15/7/20

#include "Networking.h"

#pragma comment(lib,"Ws2_32.lib")
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

#include <regex>


/// <summary>Creates a thread to call functions on.</summary>
WorkerThread::WorkerThread()
{
    wantExit = false;
    thread = std::unique_ptr<std::thread>(new std::thread(std::bind(&WorkerThread::Entry, this)));
}


/// <summary>Waits for the thread to finish and removes it.</summary>
WorkerThread::~WorkerThread()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        wantExit = true;
        queuePending.notify_one();
    }
    thread->join();
}


/// <summary>Adds job_t's to the job queue to be execute on a sepperate thread.</summary>
/// <param name="job">function to be executed</param>
void WorkerThread::addJob(job_t job)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    jobQueue.push_back(job);
    queuePending.notify_one();
}


/// <summary>Main loop of the thread.</summary>
void WorkerThread::Entry()
{
    job_t job;
    while (true) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queuePending.wait(lock, [&]() { return wantExit || !jobQueue.empty(); });

            if (wantExit) {
                return;
            }

            job = jobQueue.front();
            jobQueue.pop_front();
        }

        job();
    }
}


/// <summary>Checks whether the given IP is a valid ipv4.</summary>
/// <param name="addr">IP to validate</param>
/// <returns>Bool with is the IP is a valid ipv4 address</returns>
bool Networking::isValidIPv4(const char* IPAddr)
{
    const std::regex ipv4_regex("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");
    std::string string = IPAddr;
    std::smatch match;

    if (std::regex_match(string, match, ipv4_regex, std::regex_constants::match_flag_type::format_default)) {
        return true;
    }

    return false;
}


/// <summary>Checks whether the given IP is not an internal ipv4.</summary>
/// <param name="IPAddr">IP to validate</param>
/// <returns>Bool with is the IP is not an internal ipv4 address</returns>
bool Networking::isInternalIPv4(const char* IPAddr)
{
    // 192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
    if (std::strncmp(IPAddr, "192.168.", 8) == 0) {
        return true;
    }
    // 10.0.0.0        -   10.255.255.255  (10/8 prefix)
    if (std::strncmp(IPAddr, "10.", 3) == 0) {
        return true;
    }
    // 172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
    if (std::strncmp(IPAddr, "172.", 4) == 0) {
        int i = atoi(IPAddr + 4);
        if ((16 <= i) && (i <= 31)) {
            return true;
        }
    }

    return false;
}


/// <summary>Checks whether the given IP is in the Hamachi ipv4 address space.</summary>
/// <param name="IPAddr">IP to validate</param>
/// <returns>Bool with is the IP is in the Hamachi ipv4 address space</returns>
bool Networking::isHamachiAddr(const char* IPAddr)
{
    // 25.0.0.0        -   25.255.255.255  (10/8 prefix)
    if (std::strncmp(IPAddr, "25.", 3) == 0) {
        return true;
    }

    return false;
}


/// <summary>Checks whether the given address is a valid domain name.</summary>
/// <remarks>https://regexr.com/3au3g</remarks>
/// <param name="addr">address to validate</param>
/// <returns>Bool with is the address is a valid domain name</returns>
bool Networking::isValidDomainName(const char* addr)
{
    const std::regex domainname_regex("^(?:[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\\.)+[a-z0-9][a-z0-9-]{0,61}[a-z0-9]$");
    std::string string = addr;
    std::smatch match;

    if (std::regex_match(string, match, domainname_regex, std::regex_constants::match_flag_type::format_default)) {
        return true;
    }

    return false;
}


/// <summary>Gets the internal IP address of the user.</summary>
/// <returns>Internal IP address</returns>
std::string Networking::getInternalIPAddress()
{
    int i;
    std::string internalIPAddress;

    // Variables used by GetIpAddrTable
    PMIB_IPADDRTABLE pIPAddrTable;
    DWORD dwSize = 0;
    IN_ADDR IPAddr;

    // Before calling AddIPAddress we use GetIpAddrTable to get
    // an adapter to which we can add the IP.
    pIPAddrTable = (MIB_IPADDRTABLE*)malloc(sizeof(MIB_IPADDRTABLE));

    if (pIPAddrTable) {
        // Make an initial call to GetIpAddrTable to get the
        // necessary size into the dwSize variable
        if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
            free(pIPAddrTable);
            pIPAddrTable = (MIB_IPADDRTABLE*)malloc(dwSize);
        }
    }
    if (pIPAddrTable) {
        // Make a second call to GetIpAddrTable to get the actual data we want
        if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == NO_ERROR) {
            for (i = 0; i < (int)pIPAddrTable->dwNumEntries; i++) {
                if (pIPAddrTable->table[i].wType & MIB_IPADDR_PRIMARY) {
                    IPAddr.S_un.S_addr = (ULONG)pIPAddrTable->table[i].dwAddr;
                    char ipBuf[sizeof "255.255.255.255"];
                    internalIPAddress = inet_ntop(AF_INET, &IPAddr, ipBuf, sizeof ipBuf);
                    if (!isValidIPv4(internalIPAddress.c_str()) || !isInternalIPv4(internalIPAddress.c_str())) {
                        internalIPAddress.clear();
                    }
                }
            }
        }
        free(pIPAddrTable);
        pIPAddrTable = NULL;
    }

    return internalIPAddress;
}


/// <summary>Tries to parse the external IP address from a http responce.</summary>
/// <param name="buffer">Http responce</param>
/// <returns>External IP address</returns>
std::string parseExternalIPAddressFromResponce(std::string buffer)
{
    size_t contentLengthOffset = buffer.find("Content-Length:") + 16;
    size_t contentLengthCount = buffer.find('\n', contentLengthOffset);
    int contentLength = strtol(buffer.substr(contentLengthOffset, contentLengthCount).c_str(), NULL, 10);

    size_t addrOffset = buffer.find("\r\n\r\n");
    if (addrOffset == std::string::npos) {
        return "";
    }

    return buffer.substr(addrOffset + 4, contentLength);
}


/// <summary>Tries to get the external IP address.</summary>
/// <param name="addr">Contains the external IP address when threaded</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
/// <returns>External IP address</returns>
std::string Networking::getExternalIPAddress(std::string host, std::string* addr, bool threaded)
{
    if (threaded && addr == nullptr) {
        return "";
    }
    if (threaded) {
        std::thread(getExternalIPAddress, host, addr, false).detach();
        return "";
    }

    // Initialize Winsock.
    WSADATA wsaData = { 0 };
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        return "";
    }

    // Create a socket for sending data.
    SOCKET sendSocket = INVALID_SOCKET;
    sendSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sendSocket == INVALID_SOCKET) {
        WSACleanup();
        return "";
    }

    // Set up the addrDest structure with the IP address and port of the receiver.
    addrinfo hints;
    addrinfo* result = NULL;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(host.c_str(), NULL, &hints, &result) != 0) {
        closesocket(sendSocket);
        WSACleanup();
        return "";
    }
    sockaddr_in addrDest;
    addrDest = *(sockaddr_in*)result->ai_addr;
    addrDest.sin_family = AF_INET;
    addrDest.sin_port = htons(80);

    if (connect(sendSocket, (sockaddr*)&addrDest, sizeof addrDest) == SOCKET_ERROR) {
        closesocket(sendSocket);
        WSACleanup();
        return "";
    }

    // Send a datagram to the receiver
    std::string sendBuf = "GET / HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
    if (sendto(sendSocket, sendBuf.c_str(), (int)sendBuf.size(), 0, (sockaddr*)&addrDest, (int)sizeof addrDest) == SOCKET_ERROR) {
        closesocket(sendSocket);
        WSACleanup();
        return "";
    }

    // Wait until data received or timeout.
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sendSocket, &fds);
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    iResult = select(NULL, &fds, NULL, NULL, &tv);
    if (iResult == SOCKET_ERROR) {
        closesocket(sendSocket);
        WSACleanup();
        return "";
    }
    // Connection timedout.
    else if (iResult == 0) {
        closesocket(sendSocket);
        WSACleanup();
        return "";
    }

    // Set up the addrRetDest structure for the IP address and port from the receiver.
    sockaddr_in addrRetDest;
    int addrRetDestSize = sizeof addrRetDest;
    // Recieve a datagram from the receiver.
    char recvBuf[1024];
    int recvBufLen = 1024;
    if (recvfrom(sendSocket, recvBuf, recvBufLen, 0, (sockaddr*)&addrRetDest, &addrRetDestSize) == SOCKET_ERROR) {
        closesocket(sendSocket);
        WSACleanup();
        return "";
    }

    closesocket(sendSocket);
    WSACleanup();

    std::string externalIPAddress = parseExternalIPAddressFromResponce(recvBuf);
    if (!isValidIPv4(externalIPAddress.c_str()) || isInternalIPv4(externalIPAddress.c_str())) {
        return "";
    }

    if (addr != nullptr) {
        *addr = externalIPAddress;
    }

    return externalIPAddress;
}


/// <summary>Tries ping host to see if they are reachable.</summary>
/// <param name="IP">IP address of the host</param>
/// <param name="port">Port of the server</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
/// <returns>Bool with if the host was reachable</returns>
bool Networking::pingHost(std::string IP, unsigned short port, HostStatus* result, bool threaded)
{
    if (threaded && result == nullptr) {
        return false;
    }
    if (threaded) {
        if (result != nullptr) {
            *result = HostStatus::HOST_BUSY;
        }
        std::thread(pingHost, IP, port, result, false).detach();

        return false;
    }

    // Initialize Winsock.
    WSADATA wsaData = { 0 };
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        if (result != nullptr) {
            *result = HostStatus::HOST_ERROR;
        }

        return false;
    }

    // Create a socket for sending data.
    SOCKET sendSocket = INVALID_SOCKET;
    sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sendSocket == INVALID_SOCKET) {
        WSACleanup();
        if (result != nullptr) {
            *result = HostStatus::HOST_ERROR;
        }

        return false;
    }

    // Set up the addrDest structure with the IP address and port of the receiver.
    sockaddr_in addrDest;
    addrDest.sin_family = AF_INET;
    addrDest.sin_port = htons(port);
    if (inet_pton(addrDest.sin_family, IP.c_str(), &addrDest.sin_addr.s_addr) != 1) {
        closesocket(sendSocket);
        WSACleanup();
        if (result != nullptr) {
            *result = HostStatus::HOST_ERROR;
        }

        return false;
    }

    // Send a datagram to the receiver
    const char* sendBuf = "Hey host guy are you alive?";
    if (sendto(sendSocket, sendBuf, (int)strlen(sendBuf), 0, (sockaddr*)&addrDest, (int)sizeof addrDest) == SOCKET_ERROR) {
        closesocket(sendSocket);
        WSACleanup();
        if (result != nullptr) {
            *result = HostStatus::HOST_ERROR;
        }

        return false;
    }

    // Wait until data received or timeout.
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sendSocket, &fds);
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    iResult = select(NULL, &fds, NULL, NULL, &tv);
    if (iResult == SOCKET_ERROR) {
        closesocket(sendSocket);
        WSACleanup();
        if (result != nullptr) {
            *result = HostStatus::HOST_ERROR;
        }

        return false;
    }
    // Connection timedout.
    else if (iResult == 0) {
        closesocket(sendSocket);
        WSACleanup();
        if (result != nullptr) {
            *result = HostStatus::HOST_TIMEOUT;
        }

        return false;
    }

    closesocket(sendSocket);
    WSACleanup();
    if (result != nullptr) {
        *result = HostStatus::HOST_ONLINE;
    }

    return true;
}