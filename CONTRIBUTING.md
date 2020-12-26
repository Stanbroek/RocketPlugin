## Setting up the Development Environment
Download the repository, the included submodules (`git submodule update --init --recursive`), the listed dependencies and set the `$(BAKKESMODSDK)` macro to your BakkesMod SDK location,
after which you can start the project through `source/RocketPlugin.sln`.

### Dependencies
- [BakkesMod SDK](https://github.com/bakkesmodorg/BakkesModSDK)
- [custom Dear ImGui build](https://github.com/Stanbroek/imgui)
- [{fmt}](https://github.com/fmtlib/fmt)

### Structure of the Development Environment
#### Debug
The Debug configuration builds the plugin with additional debug logs and loads the generated DLL directly in Rocket League with `bakkes_patchplugin.py`, which comes by default with the BakkesMod SDK.  
*warning: when building this will overwrite your original RocketPlugin.dll.*
#### Release
The Debug configuration builds the plugin without debug logs.
