#pragma once
#include "Modules/RocketPluginModule.h"

#include "Networking.h"

#include "cpp-httplib/httplib.h"


class MatchFileServer final : RocketPluginModule
{
    /* Server Settings */
public:
    MatchFileServer(const std::string& host, int port, int socket_flags = 0);
    ~MatchFileServer();

    MatchFileServer(MatchFileServer&&) = delete;
    MatchFileServer(const MatchFileServer&) = delete;
    MatchFileServer& operator=(MatchFileServer&&) = delete;
    MatchFileServer& operator=(const MatchFileServer&) = delete;

private:
    static void serverStatusRequest(const httplib::Request& req, httplib::Response& res);
    static void serverDownloadMapRequest(const httplib::Request& req, httplib::Response& res);

    std::thread serverThread;
    httplib::Server svr;

    /* Client Settings */
public:
    struct ServerStatus
    {
        bool Online = false;
        std::string Version;
        struct MapStatus
        {
            std::string MapName;
            size_t FileSize = 0;
            std::string Guid = "00000000 00000000 00000000 00000000";
            bool Loaded = false;
            bool Override = false;
        };
        MapStatus CurrentMap;
        std::vector<MapStatus> Dependencies;

        bool ShouldDownloadMap() const { return Online && CurrentMap.FileSize > 0 && (!CurrentMap.Loaded || CurrentMap.Override); }
    };

    static std::future<ServerStatus> GetServerStatus(const std::string& host, int port, Networking::HostStatus* hostStatus = nullptr);
    static std::future<bool> DownloadMap(const std::string& host, int port, const std::filesystem::path& filePath, float* downloadStatus = nullptr);
};
