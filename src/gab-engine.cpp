// gab-engine.cpp : Defines the entry point for the application.
//

#include "gab-engine.h"

#if _WIN32 || _WIN64
#include <Windows.h>
#endif // _WIN32 || _WIN64


int main(int argc, char* argv[])
{
    //===================PATH SETS=================//
#if _WIN32 || _WIN64
    auto path = std::filesystem::current_path() / "vcpkg_installed" /
        "x64-windows" / "bin";
    if (!SetEnvironmentVariable("VK_ADD_LAYER_PATH", path.string().c_str())) {
        std::cerr << "Failed to set VK_ADD_LAYER_PATH environment variable." << std::endl;
    }
#endif // _WIN32 || _WIN64

    auto engine = new gbe::Engine();
    engine->Run();

    return 0;
}