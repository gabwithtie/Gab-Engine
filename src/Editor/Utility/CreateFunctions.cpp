#include "CreateFunctions.h"
#include "Engine/gbe_engine.h"

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
}