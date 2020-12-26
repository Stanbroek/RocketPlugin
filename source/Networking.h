#pragma once
#include <functional>
#include <vector>
#include <string>
#include <thread>
#include <mutex>

#include "utils/stringify.h"
#include "utils/cvarmanagerwrapperdebug.h"

#define DELETE_DEFAULTED_FUNCTIONS(cls) \
    cls(const cls&) = delete; \
    cls(cls&&) = delete; \
    cls& operator=(const cls&) = delete; \
    cls& operator=(cls&&) = delete


namespace Networking
{
    enum class DestAddrType
    {
        UNKNOWN_ADDR,
        PRIVATE_ADDR,
        INTERNL_ADDR,
        HAMACHI_ADDR,
        EXTERNL_ADDR
    };

    enum class HostStatus
    {
        HOST_UNKNOWN,
        HOST_BUSY,
        HOST_ERROR,
        HOST_TIMEOUT,
        HOST_ONLINE,
    };

    DestAddrType GetDestAddrType(const char* addr);
    std::string GetHostStatusHint(DestAddrType addrType, HostStatus hostStatus);

    std::string IPv4ToString(const void* addr);

    bool IsValidPort(int port);
    bool IsValidIPv4(const std::string& ipAddr);
    bool IsPrivateIPv4(const std::string& ipAddr);
    bool IsExternalIPv4(const std::string& ipAddr);
    bool IsHamachiIPv4(const std::string& ipAddr);
    bool IsValidDomainName(const std::string& addr);

    std::error_code NetworkRequest(const std::string& host, unsigned short port, int protocol, const char* sendBuf,
                                   size_t sendBufSize, char* recvBuf = nullptr, size_t recvBufSize = 0);
    std::error_code GetInternalIPAddress(std::string& ipAddr);
    std::error_code GetExternalIPAddress(const std::string& host, std::string* ipAddr, bool threaded = false);

    bool PingHost(const std::string& host, unsigned short port, HostStatus* result = nullptr, bool threaded = false);
}


/// <summary>Threaded class that queues <see cref="job_t"/> to be executed.</summary>
/// <remarks> From http://www.cplusplus.com/forum/general/87747/ </remarks>
class WorkerThread
{
public:
    typedef std::function<void()> job_t;

    WorkerThread();
    ~WorkerThread();
    DELETE_DEFAULTED_FUNCTIONS(WorkerThread);

    void addJob(const job_t& job);
    void entry();
private:
    std::thread thread;
    std::condition_variable queuePending;
    std::mutex queueMutex;
    std::list<job_t> jobQueue;
    bool wantExit;
};


// Predefine types without including them.
struct IUPnPService;
struct IUPnPDevice;

class UPnPClient
{
public:
    UPnPClient();
    ~UPnPClient();
    DELETE_DEFAULTED_FUNCTIONS(UPnPClient);

    void findDevices(bool threaded = true);
    std::string getDiscoveryStatus();
    bool discoverySearching() const { return discoveryStatus == DiscoveryStatus::DISCOVERY_BUSY; }
    bool discoveryFailed() const { return discoveryStatus == DiscoveryStatus::DISCOVERY_ERROR; }
    bool discoveryFinished() const { return discoveryStatus == DiscoveryStatus::DISCOVERY_FINISHED; }

    void forwardPort(unsigned short internalPort, unsigned short externalPort, unsigned long portLeaseDuration,
                     bool threaded = true);
    std::string getForwardPortStatus() const;
    bool serviceForwardPortFailed() const { return addPortMappingStatus == ServiceStatus::SERVICE_ERROR; }
    bool serviceForwardPortActive() const { return addPortMappingStatus == ServiceStatus::SERVICE_BUSY; }
    bool serviceForwardPortFinished() const { return addPortMappingStatus == ServiceStatus::SERVICE_UPDATED_PORT_MAPPING; }

    void closePort(unsigned short externalPort, bool threaded = true);
    std::string getClosePortStatus() const;
    bool serviceClosePortFailed() const { return deletePortMappingStatus == ServiceStatus::SERVICE_ERROR; }
    bool serviceClosePortActive() const { return deletePortMappingStatus == ServiceStatus::SERVICE_BUSY; }
    bool serviceClosePortFinished() const { return deletePortMappingStatus == ServiceStatus::SERVICE_UPDATED_PORT_MAPPING; }

    std::vector<unsigned short> getOpenPorts() const { return openPorts; }
    char* getExternalIPAddressBuffer() const { return const_cast<char*>(externalIPAddress.data()); }

private:
    enum class DiscoveryStatus
    {
        DISCOVERY_IDLE,
        DISCOVERY_ERROR,
        DISCOVERY_BUSY,
        DISCOVERY_FINISHED,
    };

    enum class ServiceStatus
    {
        SERVICE_IDLE,
        SERVICE_ERROR,
        SERVICE_BUSY,
        SERVICE_GOT_EXT_IP,  // Set by findDevices() after GTO service has been found.
        SERVICE_UPDATED_PORT_MAPPING,  // Set when forwardPort() or closePort() is finished.
        SERVICE_FINISHED,  // Set when findOpenPorts() is finished.
    };

    void discoverDevices(const std::wstring& typeUri);
    void findOpenPorts(bool threaded = true);
    std::string getDeviceFriendlyName(IUPnPDevice* pDevice);
    void clearDevices();

    std::unique_ptr<WorkerThread> discoverThread;
    IUPnPService* gtoServices[2] = {nullptr, nullptr};
    IUPnPDevice* gtoDevice = nullptr;
    std::string deviceFriendlyName;

    std::error_code discoveryResult;
    std::string discoveryReturnStatus;
    DiscoveryStatus discoveryStatus = DiscoveryStatus::DISCOVERY_IDLE;

    std::error_code addPortMappingResult;
    std::string addPortMappingReturnStatus;
    ServiceStatus addPortMappingStatus = ServiceStatus::SERVICE_IDLE;

    std::error_code deletePortMappingResult;
    std::string deletePortMappingReturnStatus;
    ServiceStatus deletePortMappingStatus = ServiceStatus::SERVICE_IDLE;

    std::string internalIPAddress;
    std::string externalIPAddress = "Not found your external IP address yet.";
    std::vector<unsigned short> openPorts;
};


class P2PHost
{
public:
    P2PHost();
    ~P2PHost() = default;
    DELETE_DEFAULTED_FUNCTIONS(P2PHost);

    void findNATType(unsigned short port, bool threaded = true);
    void punchPort(const std::string& ip, unsigned short port, bool threaded = true);
    std::string getNATDesc() const;

    enum class NATType
    {
        NAT_NONE,
        NAT_SEARCHING,
        NAT_BLOCKED,
        NAT_FULL_CONE,
        NAT_RESTRICTED,
        NAT_RESTRICTED_PORT,
        NAT_SYMMETRIC,
        NAT_ERROR
    };

    NATType getNATType() const { return natType; }

private:
    std::unique_ptr<WorkerThread> discoverThread;
    NATType natType = NATType::NAT_NONE;
    std::error_code lastError;
};
