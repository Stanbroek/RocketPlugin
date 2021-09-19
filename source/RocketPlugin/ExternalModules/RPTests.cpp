#include "RocketPlugin.h"

#include "ExternalModules.h"


/*
 *  Test stuff
 */

RP_EXTERNAL_DEBUG_NOTIFIER("rp_test_paths", [](const std::vector<std::string>&) {
    BM_INFO_LOG(absolute(BINDS_FILE_PATH).string());
    BM_INFO_LOG(absolute(CONFIG_FILE_PATH).string());
    BM_INFO_LOG(absolute(PRESETS_PATH).string());
    BM_INFO_LOG(absolute(PRO_TIPS_FILE_PATH).string());
    BM_INFO_LOG(absolute(CUSTOM_MAPS_PATH).string());
    BM_INFO_LOG(absolute(WORKSHOP_MAPS_PATH).string());
}, "Logs paths", PERMISSION_ALL); }


RP_EXTERNAL_DEBUG_NOTIFIER("rp_test_logs", [](const std::vector<std::string>&) {
    BM_LOG("normal log");
    BM_TRACE_LOG("trace log");
    BM_INFO_LOG("info log");
    BM_WARNING_LOG("warning log");
    BM_ERROR_LOG("error log");
    BM_CRITICAL_LOG("critical log");
}, "Logs a bunch", PERMISSION_ALL); }


RP_EXTERNAL_DEBUG_NOTIFIER("rp_crash", [](const std::vector<std::string>&) {
    *static_cast<unsigned*>(nullptr) = 0;
}, "Crashes by dereferencing nullptr", PERMISSION_ALL); }


RP_EXTERNAL_DEBUG_NOTIFIER("rp_throw", [](const std::vector<std::string>&) {
    throw std::runtime_error("NotifierInterrupt");
}, "Throws exception", PERMISSION_ALL); }
