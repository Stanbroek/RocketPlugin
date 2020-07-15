#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>


class Networking
{
public:
    enum class HostStatus
    {
        HOST_UNKNOWN,
        HOST_BUSY,
        HOST_ERROR,
        HOST_TIMEOUT,
        HOST_ONLINE,
    };

    static bool isValidIPv4(const char* IPAddr);
    static bool isInternalIPv4(const char* IPAddr);
    static bool isHamachiAddr(const char* IPAddr);
    static bool isValidDomainName(const char* addr);
    static std::string getInternalIPAddress();
    static std::string getExternalIPAddress(std::string host, std::string* IPAddr = nullptr, bool threaded = false);
    static bool pingHost(std::string IP, unsigned short port, HostStatus* result = nullptr, bool threaded = false);
};


/// <summary>Threaded class that queues <see cref="job_t"/> to be executed.</summary>
/// <remarks> From http://www.cplusplus.com/forum/general/87747/ </remarks>
class WorkerThread
{
public:
    typedef std::function<void()> job_t;

    WorkerThread();
    ~WorkerThread();
    void addJob(job_t job);
    void Entry();
private:
    std::unique_ptr<std::thread> thread;
    std::condition_variable      queuePending;
    std::mutex                   queueMutex;
    std::list<job_t>             jobQueue;
    bool                         wantExit;
};


// Predefine types.
struct IUPnPServices;
struct IUPnPService;
struct IUPnPDevices;
struct IUPnPDevice;
#ifndef _HRESULT_DEFINED
    typedef long HRESULT;
#endif // !_HRESULT_DEFINED

class UPnPClient
{
private:
    enum class DiscoveryStatus
    {
        DISCOVERY_IDLE,
        DISCOVERY_ERROR,
        DISCOVERY_BUSY,
        DISCOVERY_FOUND_GTO,
        DISCOVERY_FINISHED,
    };
    enum class ServiceStatus
    {
        SERVICE_IDLE,
        SERVICE_ERROR,
        SERVICE_BUSY,
        SERVICE_GOT_EXT_IP,
        SERVICE_ADDED_PORT_MAPPING,
        SERVICE_FINISHED,
    };

    HRESULT     TraverseServiceCollection(IUPnPServices* pusServices, bool saveServices = false);
    HRESULT     TraverseDeviceCollection(IUPnPDevices* pDevices, unsigned int depth = 0, bool saveDevice = false);
    void        DiscoverDevices(std::wstring TypeURI);
    HRESULT     getInternalIPAddress();
    std::string getDeviceFriendlyName(IUPnPDevice* pDevice);
    void        clearDevices();

    WorkerThread*   discoverThread = nullptr;
    IUPnPService*   GTOServices[2] = { nullptr, nullptr };
    IUPnPDevice*    GTODevice      = nullptr;
    DiscoveryStatus discoveryStatus                = DiscoveryStatus::DISCOVERY_IDLE;
    ServiceStatus   serviceAddPortMappingStatus    = ServiceStatus::SERVICE_IDLE;
    ServiceStatus   serviceDeletePortMappingStatus = ServiceStatus::SERVICE_IDLE;
    std::string     deviceFriendlyName;
    std::string     findDevicesStatus;
    std::string     forwardPortStatus;
    std::string     closePortStatus;
    HRESULT	        hResult = 0;

    std::string    internalIPAddress;
    unsigned short internalPort = 7777;

public:
    bool discoveryFailed()         { return discoveryStatus == DiscoveryStatus::DISCOVERY_ERROR; };
    bool discoverySearching()      { return discoveryStatus == DiscoveryStatus::DISCOVERY_BUSY ||
                                            discoveryStatus == DiscoveryStatus::DISCOVERY_FOUND_GTO; };
    bool discoveryFinished()       { return discoveryStatus == DiscoveryStatus::DISCOVERY_FINISHED; };
    bool serviceAddPortFailed()    { return serviceAddPortMappingStatus == ServiceStatus::SERVICE_ERROR; };
    bool serviceAddPortActive()    { return serviceAddPortMappingStatus == ServiceStatus::SERVICE_BUSY; };
    bool serviceDeletePortFailed() { return serviceDeletePortMappingStatus == ServiceStatus::SERVICE_ERROR; };
    bool serviceDeletePortActive() { return serviceDeletePortMappingStatus == ServiceStatus::SERVICE_BUSY; };
    
    void forwardPort(unsigned short externalPort, unsigned long portLeaseDuration, bool threaded = true);
    void closePort(unsigned short externalPort, bool threaded = true);
    void findOpenPorts(bool threaded = true);
    void findDevices(bool threaded = true);
    
    std::string getDiscoveryStatus();
    std::string getforwardPortStatus();
    std::string getclosePortStatus();

    UPnPClient();
    ~UPnPClient();

    std::vector<unsigned short> openPorts;
    char externalIPAddress[64] = "Not found your external IP address yet.";
};


class P2PHost
{
public:
    enum class NATType
    {
        NAT_YET_DETERMINED,
        NAT_SEARCHING,
        NAT_BLOCKED,
        NAT_FULL_CONE,
        NAT_RESTRIC,
        NAT_RESTRIC_PORT,
        NAT_SYMMETRIC,
        NAT_ERROR
    };
private:
    std::unique_ptr<WorkerThread> discoverThread;
    NATType natType = NATType::NAT_YET_DETERMINED;
public:
    void findNATType(unsigned short port, bool threaded = true);
    void punchPort(std::string IP, unsigned short port, bool threaded = true);
    std::string getNATDesc();
    NATType     getNATType();
    
    P2PHost();
};