// MatchFileServer.cpp
// Match file server for the RocketPlugin plugin.
//
// Author:       Stanbroek
// Version:      0.6.9.1 31/10/21
//
// References:
//  https://github.com/yhirose/cpp-httplib
//  https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
//  https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types

#include "MatchFileServer.h"
#include "RocketPlugin.h"


MatchFileServer::MatchFileServer(const std::string& host, const int port, const int socket_flags)
{
    serverThread = save_thread("MatchFileServer", [this, host, port, socket_flags]() {
        BM_INFO_LOG("Starting match file server on {:s}:{:d}", host, port);
        svr.Get("/status", serverStatusRequest);
        svr.Get("/map", serverDownloadMapRequest);

        if (!svr.bind_to_port(host.c_str(), port, socket_flags)) {
            BM_ERROR_LOG("Failed to bind match file server to port {:d}", port);
            return;
        }

        if (svr.listen_after_bind()) {
            BM_INFO_LOG("Stopping match file server");
        }
        else {
            BM_ERROR_LOG("Failed to start match file server");
        }
    });
}


MatchFileServer::~MatchFileServer()
{
    svr.stop();
    if (serverThread.joinable()) {
        serverThread.join();
    }
}


std::string get_errno_str(errno_t error)
{
    char errmsg[128];
    if (strerror_s(errmsg, error)) {
        return fmt::format("unknown error {:X}", error);
    }
    
    return errmsg;
}


size_t get_file_size(const std::filesystem::path& file)
{
    if (file.empty()) {
        return 0;
    }

    struct _stati64 stats{};
    if (_wstati64(file.c_str(), &stats) == -1) {
        BM_ERROR_LOG("wstati64: {:s}", get_errno_str(errno));
        return 0;
    }

    return stats.st_size;
}


std::filesystem::path get_map_file_path(const std::wstring&)
{
    BM_WARNING_LOG("redacted function");

    return {};
}


std::string GetCurrentMapName()
{
    BM_WARNING_LOG("redacted function");

    return {};
}


std::string GetCurrentMapGuid()
{
    BM_WARNING_LOG("redacted function");

    return {};
}


void MatchFileServer::serverStatusRequest(const httplib::Request& req, httplib::Response& res)
{
    BM_TRACE_LOG("incomming request from {:s}:{:d}", req.remote_addr, req.remote_port);

    if (!Outer()->IsHostingLocalGame()) {
        BM_ERROR_LOG("Could not find local game.");
        res.status = 503;  // Service Unavailable.
        return;
    }
    
    const std::string currentMap = GetCurrentMapName();
    const std::string currentMapGuid = GetCurrentMapGuid();

    const std::filesystem::path filePath = get_map_file_path(to_wstring(currentMap));
    const size_t fileSize = get_file_size(filePath);
    if (fileSize == 0) {
        BM_ERROR_LOG("{:s} could not be loaded.", quote(currentMap));
        res.status = 503;  // Service Unavailable.
        return;
    }

    const std::string body = fmt::format(R"({{"Version": {:s}, "CurrentMap": {:s}, "FileSize": {:d}, "Guid": {:s}}})",
        quote(PLUGIN_VERSION), quote(currentMap), fileSize, quote(currentMapGuid));
    BM_TRACE_LOG("response: {:s}", body);

    res.set_content(body, "application/json");
}


void MatchFileServer::serverDownloadMapRequest(const httplib::Request& req, httplib::Response& res)
{
    BM_TRACE_LOG("incomming request from {:s}:{:d}", req.remote_addr, req.remote_port);

    if (Outer()->GetGame().IsNull()) {
        BM_ERROR_LOG("Could not find local game.");
        res.status = 503;  // Service Unavailable.
        return;
    }

    const std::string currentMap = GetCurrentMapName();
    if (currentMap.empty()) {
        BM_ERROR_LOG("Could not find current map.");
        res.status = 503;  // Service Unavailable.
        return;
    }

    const std::filesystem::path filePath = get_map_file_path(to_wstring(currentMap));
    if (filePath.empty()) {
        BM_ERROR_LOG("Current map file could not be loaded.");
        res.status = 503;  // Service Unavailable.
        return;
    }

    int fd = 0;
    const errno_t error = _wsopen_s(&fd, filePath.c_str(), _O_RDONLY | _O_BINARY, _SH_DENYNO, NULL);
    if (error) {
        BM_ERROR_LOG("Could not open the current map file");
        BM_DEBUG_ERROR_LOG("_wsopen_s: {:s}", get_errno_str(error));
        res.status = 500;  // Internal Server Error
        return;
    }

    struct _stati64 stats{};
    if (_fstati64(fd, &stats) == -1) {
        BM_ERROR_LOG("Could not access the current map file");
        BM_DEBUG_ERROR_LOG("_fstati64: {:s}", get_errno_str(errno));
        res.status = 500;  // Internal Server Error
        return;
    }

    res.set_content_provider(
        stats.st_size,  // Content length
        "application/octet-stream",  // Content type
        [fd](size_t /*offset*/, const size_t length, httplib::DataSink& sink) {
            //_seek(fd, offset);
            std::vector<char> data(length);
            if (_read(fd, data.data(), static_cast<unsigned int>(length)) <= 0) {
                BM_ERROR_LOG("Could not read the current map file");
                BM_DEBUG_ERROR_LOG("_read: {:s}", get_errno_str(errno));
                return false;
            }
            sink.write(data.data(), length);
            return true; // return 'false' if you want to cancel the process.
        },
        [fd](bool /*success*/) { _close(fd); }
    );

    BM_TRACE_LOG("uploading: {:s}", currentMap);
}


std::future<MatchFileServer::ServerStatus> MatchFileServer::GetServerStatus(const std::string& host, const int port, Networking::HostStatus* hostStatus)
{
    return save_promise<ServerStatus>("GetServerStatus", [=]() -> ServerStatus {
        BM_TRACE_LOG("Requesting server status");
        ServerStatus serverStatus;
        if (!Networking::PingHost(host, static_cast<unsigned short>(port), hostStatus).get()) {
            BM_ERROR_LOG("Could not reach the host");
            return serverStatus;
        }

        httplib::Client cli(host, port);

        const httplib::Result res = cli.Get("/status");
        if (res.error() != httplib::Error::Success || res == nullptr) {
            BM_ERROR_LOG("Could not get the host status: {:s}", httplib::to_string(res.error()));
            return serverStatus;
        }
        if (res->status != 200) {
            BM_ERROR_LOG("Host status returned: {:s}", httplib::detail::status_message(res->status));
            return serverStatus;
        }

        BM_TRACE_LOG("{:s}", res->body);

        serverStatus.Online = true;

        simdjson::ondemand::parser parser;
        const simdjson::padded_string paddedData = res->body;
        simdjson::ondemand::document doc = parser.iterate(paddedData);

        const auto version = doc["Version"].get_string();
        if (version.error()) {
            BM_ERROR_LOG("Version was not provided in server status.\n{:s}", simdjson::error_message(version.error()));
            return serverStatus;
        }
        serverStatus.Version = version.value_unsafe();
        if (serverStatus.Version != PLUGIN_VERSION) {
            BM_WARNING_LOG("Server version does not match your clients version, {:s} != {:s}", serverStatus.Version, PLUGIN_VERSION);
        }

        const auto currentMap = doc["CurrentMap"].get_string();
        if (currentMap.error()) {
            BM_ERROR_LOG("Current map was not provided in server status.\n{:s}", simdjson::error_message(currentMap.error()));
            return serverStatus;
        }
        serverStatus.CurrentMap.MapName = currentMap.value_unsafe();

        const auto fileSize = doc["FileSize"].get_uint64();
        if (fileSize.error()) {
            BM_ERROR_LOG("File size was not provided in server status.\n{:s}", simdjson::error_message(fileSize.error()));
            return serverStatus;
        }
        serverStatus.CurrentMap.FileSize = fileSize.value_unsafe();

        const auto guid = doc["Guid"].get_string();
        if (guid.error()) {
            BM_ERROR_LOG("File guid was not provided in server status.\n{:s}", simdjson::error_message(guid.error()));
            return serverStatus;
        }
        serverStatus.CurrentMap.Guid = guid.value_unsafe();

        BM_WARNING_LOG("redacted function");
        serverStatus.CurrentMap.Loaded = false;

        return serverStatus;
    });
}


std::future<bool> MatchFileServer::DownloadMap(const std::string& host, const int port, const std::filesystem::path& filePath, float* downloadStatus)
{
    return save_promise<bool>("DownloadMap", [=]() -> bool {
        BM_TRACE_LOG("Requesting server map");
        std::ofstream file(filePath, std::ios::trunc | std::ios::binary);
        if (!file.is_open()) {
            BM_ERROR_LOG("Could not open file: {:s}", filePath.string());
            return false;
        }

        httplib::Client cli(host, port);

        httplib::Result res = cli.Get("/map",
            [&file](const char* data, const size_t data_length) -> bool {
                file.write(data, data_length);
                return !file.fail();
            },
            [downloadStatus](const uint64_t current, const uint64_t total) -> bool {
                BM_TRACE_LOG("{:d}/{:d}", current, total);
                if (downloadStatus != nullptr) {
                    *downloadStatus = static_cast<float>(current) / static_cast<float>(total);
                }
                return true;
            }
        );
        if (res.error() != httplib::Error::Success || res == nullptr) {
            BM_ERROR_LOG("Could not get the host status: {:s}", httplib::to_string(res.error()));
            return false;
        }
        if (res->status != 200) {
            BM_ERROR_LOG("Host status returned: {:s}", httplib::detail::status_message(res->status));
            return false;
        }
        BM_TRACE_LOG("Map download completed");

        BM_WARNING_LOG("redacted function");
        BM_ERROR_LOG("Could not load the map, restart your game to load the map.");
        return false;
    });
}
