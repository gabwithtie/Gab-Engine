#include "SpawnWindow.h"

#include <typeinfo>

void gbe::editor::SpawnWindow::DrawSelf()
{
	ImGui::Text("Spawned object will parented to the currently selected object.");

	Object* parent = Engine::GetCurrentRoot();

	if (this->selected.size() > 0)
		parent = this->selected[0];

	Object* newobj = nullptr;

	ImGui::PushID("Renderers");
	
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
		newobj->PushEditorFlag(Object::SERIALIZABLE);
		newobj->SetParent(parent);
		Console::Log("New [" + std::string(typeid(*newobj).name()) + "] parented to a [" + std::string(typeid(*parent).name()) + "]");
	}
}

std::string gbe::editor::SpawnWindow::GetWindowId()
{
	return "Spawning";
}
