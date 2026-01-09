// gab-engine.cpp : Defines the entry point for the application.
//

#include "gab-engine.h"
#include "AnitoBuilderExtension.h"

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

	auto anito_builder_extension = new gbe::ext::AnitoBuilder::AnitoBuilderExtension();

    std::vector<gbe::Extension*> extensions = {
		(gbe::Extension*)anito_builder_extension
    };

    auto engine = new gbe::Engine(extensions);
    engine->Run();

    return 0;
}