#include "CreateFunctions.h"
#include "Engine/gbe_engine.h"

namespace gbe::editor {
	const std::unordered_map<std::string, std::function<Object* ()>> CreateFunctions::createfunctions = {
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
}