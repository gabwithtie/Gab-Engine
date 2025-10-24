#include "CreateFunctions.h"
#include "Engine/gbe_engine.h"
#include "Ext/AnitoBuilderWrapper/BuilderBlock.h"

namespace gbe::editor {
	const std::unordered_map<std::string, std::function<Object* ()>> CreateFunctions::createfunctions_primitives = {
		{
			"Cube"
			,
			[]() {
			
			auto pos = Engine::GetActiveCamera()->World().position.Get() + Engine::GetActiveCamera()->World().GetForward() * 5.0f;
			
			auto object = new RenderObject(RenderObject::cube);
			object->World().position.Set(pos);
			object->SetName("New Cube");

			return object;
			}
		}
	};
	const std::unordered_map<std::string, std::function<Object* ()>> CreateFunctions::createfunctions_light = {
		{
			ObjectNamer::GetName(typeid(DirectionalLight))
			,
			[]() {

			auto pos = Engine::GetActiveCamera()->World().position.Get() + Engine::GetActiveCamera()->World().GetForward() * 5.0f;

			auto object = new DirectionalLight();
			object->World().position.Set(pos);
			object->SetName("New Directional Light");
			object->Set_Color(Vector3(1));

			return object;
			}
		},{
			ObjectNamer::GetName(typeid(ConeLight))
			,
			[]() {

			auto pos = Engine::GetActiveCamera()->World().position.Get() + Engine::GetActiveCamera()->World().GetForward() * 5.0f;

			auto object = new ConeLight();
			object->World().position.Set(pos);
			object->SetName("New Cone Light");
			object->Set_Color(Vector3(1));

			return object;
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
			auto object = new ext::AnitoBuilder::BuilderBlock(cubecorners, 4);

			return object;
			}
		}
	};
}