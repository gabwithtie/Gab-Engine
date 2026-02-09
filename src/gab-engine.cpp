// gab-engine.cpp : Defines the entry point for the application.
//

#include "gab-engine.h"

//Backends
#include "bgfx-gab/bgfx_gab.h"

//Extensions
#include "AnitoBuilderExtension.h"

int main(int argc, char* argv[])
{
    //Backends
	auto bgfx_backend = new gbe::gfx::bgfx_gab::bgfx_gab();

    //Editor Extensions
	auto anito_builder_extension = new gbe::ext::AnitoBuilder::AnitoBuilderExtension();

    std::vector<gbe::Extension*> extensions = {
		(gbe::Extension*)anito_builder_extension
    };

    //Main
    auto engine = new gbe::Engine(extensions);
    engine->Run();

    return 0;
}