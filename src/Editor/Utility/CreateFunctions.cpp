#include "CreateFunctions.h"
#include "Engine/gbe_engine.h"
#include "Ext/AnitoBuilderWrapper/BuilderBlock.h"

namespace gbe::editor {
	const std::unordered_map<std::string, std::function<Object* ()>> CreateFunctions::createfunctions_light = {
		{
			ObjectNamer::GetName(typeid(DirectionalLight))
			,
			[]() {
			auto dirlight = new DirectionalLight();
			dirlight->World().position.Set(Vector3(0, 0, -10));
			dirlight->SetName("New Directional Light");
			dirlight->Set_Color(Vector3(1));

			return dirlight;
			}
		},{
			ObjectNamer::GetName(typeid(ConeLight))
			,
			[]() {
			auto dirlight = new ConeLight();
			dirlight->World().position.Set(Vector3(0, 0, -10));
			dirlight->SetName("New Cone Light");
			dirlight->Set_Color(Vector3(1));

			return dirlight;
			}
		}
	};
	const std::unordered_map<std::string, std::function<Object* ()>> CreateFunctions::createfunctions_ext_anitobuilder = {
		{
			ObjectNamer::GetName(typeid(ext::AnitoBuilder::BuilderBlock))
			,
			[]() {
			Vector3 cubecorners[4] = {
			Vector3(-2, 0, -2),
			Vector3(2, 0, -2),
			Vector3(2, 0, 2),
			Vector3(-2, 0, 2),
			};
			auto builder_cube = new ext::AnitoBuilder::BuilderBlock(cubecorners, 4);

			return builder_cube;
			}
		}
	};
}