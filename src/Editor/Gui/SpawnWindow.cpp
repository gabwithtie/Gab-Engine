#include "SpawnWindow.h"

#include <typeinfo>

void gbe::editor::SpawnWindow::DrawSelf()
{
	ImGui::Text("Spawned object will parented to the currently selected object.");

	Object* parent = Engine::GetCurrentRoot();

	if ((*this->selected).size() > 0)
		parent = (*this->selected)[0];

	Object* newobj = nullptr;

	ImGui::SeparatorText("RigidObject Root");

	if (ImGui::Button("Dynamic RigidObject")) {
		newobj = new RigidObject(false);
	}
	ImGui::SameLine();
	if (ImGui::Button("Static RigidObject")) {
		newobj = new RigidObject(true);
	}

	ImGui::SeparatorText("Collider");

	if (ImGui::Button("Cube")) {
		newobj = new BoxCollider();
	}
	ImGui::SameLine();
	if (ImGui::Button("Sphere")) {
		newobj = new SphereCollider();
	}
	ImGui::SameLine();
	if (ImGui::Button("Capsule")) {
		newobj = new CapsuleCollider();
	}

	ImGui::PushID("renderer_buttons");
	
	ImGui::SeparatorText("Renderer");

	if (ImGui::Button("Cube")) {
		newobj = new RenderObject(RenderObject::cube);
	}
	ImGui::SameLine();
	if (ImGui::Button("Sphere")) {
		newobj = new RenderObject(RenderObject::sphere);
	}
	ImGui::SameLine();
	if (ImGui::Button("Plane")) {
		newobj = new RenderObject(RenderObject::plane);
	}
	ImGui::SameLine();
	if (ImGui::Button("Capsule")) {
		newobj = new RenderObject(RenderObject::capsule);
	}
	ImGui::PopID();

	if (newobj != nullptr) {
		newobj->SetParent(parent);
		Console::Log("New [" + std::string(typeid(*newobj).name()) + "] parented to a [" + std::string(typeid(*parent).name()) + "]");
	}
}

std::string gbe::editor::SpawnWindow::GetWindowId()
{
	return "Spawning";
}
