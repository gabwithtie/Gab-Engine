// gab-engine.cpp : Defines the entry point for the application.
//

#include "gab-engine.h"

int main(int argc, char* argv[])
{
    //PATH SETS
    auto path = std::filesystem::current_path() / "vcpkg_installed" /
        "x64-windows" / "bin";
    std::string set = "VK_ADD_LAYER_PATH=" + path.string();
    _putenv(set.c_str());

    auto engine = new gbe::Engine();
    engine->Run();

    return 0;
}