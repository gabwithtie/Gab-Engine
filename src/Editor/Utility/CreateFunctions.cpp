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
			object->PushEditorFlag(Object::SERIALIZABLE);
			object->SetUserCreated();

			return object;
			}
		},
		{
			"Sphere"
			,
			[]() {

			auto pos = Engine::GetActiveCamera()->World().position.Get() + Engine::GetActiveCamera()->World().GetForward() * 5.0f;

			auto object = new RenderObject(RenderObject::sphere);
			object->World().position.Set(pos);
			object->SetName("New Sphere");
			object->PushEditorFlag(Object::SERIALIZABLE);
			object->SetUserCreated();

			return object;
			}
		},
		{
			"Plane"
			,
			[]() {

			auto pos = Engine::GetActiveCamera()->World().position.Get() + Engine::GetActiveCamera()->World().GetForward() * 5.0f;

			auto object = new RenderObject(RenderObject::plane);
			object->World().position.Set(pos);
			object->SetName("New Plane");
			object->PushEditorFlag(Object::SERIALIZABLE);
			object->SetUserCreated();

			return object;
			}
		},
	};
	const std::unordered_map<std::string, std::function<Object* ()>> CreateFunctions::createfunctions_light = {
		{
			"Directional"
			,
			[]() {

			auto pos = Engine::GetActiveCamera()->World().position.Get() + Engine::GetActiveCamera()->World().GetForward() * 5.0f;

			auto object = new DirectionalLight();
			object->World().position.Set(pos);
			object->SetName("New Directional Light");
			object->Set_Color(Vector3(1));

			return object;
			}
		}
		,{
			"Cone"
			,
			[]() {

			auto pos = Engine::GetActiveCamera()->World().position.Get() + Engine::GetActiveCamera()->World().GetForward() * 5.0f;

			auto object = new ConeLight();
			object->World().position.Set(pos);
			object->SetName("New Cone Light");
			object->Set_Color(Vector3(1));

			return object;
			}
		},{
			"Point"
			,
			[]() {

			auto pos = Engine::GetActiveCamera()->World().position.Get() + Engine::GetActiveCamera()->World().GetForward() * 5.0f;

			auto object = new PointLight();
			object->World().position.Set(pos);
			object->SetName("New Point Light");
			object->Set_Color(Vector3(1));

			return object;
			}
		}
	};
	const std::unordered_map<std::string, std::function<Object* ()>> CreateFunctions::createfunctions_ext_anitobuilder = {
		{
			"Anito Builder Block"
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