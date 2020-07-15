#pragma once
#include "bakkesmod/wrappers/cvarmanagerwrapper.h"


class CVarManagerWrapperDebug : public CVarManagerWrapper
{
private:
    inline void _log(std::string text)
    {
#ifdef DEBUG
        log(text);
#endif // DEBUG
    }
public:
    inline void error_log(std::string text)
    {
        _log("ERROR, " + text);
    }

    inline void info_log(std::string text)
    {
        _log("INFO, " + text);
    }
};

#undef IM_COL32_ERROR_RED
