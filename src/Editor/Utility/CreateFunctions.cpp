#include "CreateFunctions.h"

namespace gbe::editor {
	const std::unordered_map<std::string, std::function<Object*()>> CreateFunctions::createfunctions = {
				{
					typeid(DirectionalLight).name()
					,
					[]() {
					auto dirlight = new DirectionalLight();
					dirlight->World().position.Set(Vector3(0, 0, -10));
					dirlight->SetName("Directional Light 1");
					dirlight->Set_Color(Vector3(1, 0.3, 0.3));

					return dirlight;
					}
				}
	};
}