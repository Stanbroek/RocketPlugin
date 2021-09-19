#pragma once
class RocketPlugin;

class RocketPluginModule
{
    friend RocketPlugin;
public:
    static RocketPlugin* Outer()
    {
        return rocketPlugin;
    }

protected:
    static RocketPlugin* rocketPlugin;
};
