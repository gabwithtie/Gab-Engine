#include "StateWindow.h"
#include "Engine/gbe_engine.h"

void gbe::editor::StateWindow::DrawSelf()
{
	if (ImGui::Button("Play")) {
		Engine::Set_state(Engine::EngineState::Play);
	}
	ImGui::SameLine();
	if (ImGui::Button("Pause")) {
		Engine::Set_state(Engine::EngineState::Paused);
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop")) {
		Engine::Set_state(Engine::EngineState::Edit);
	}
	ImGui::SameLine();
	if (ImGui::Button("Step")) {
		Engine::Step(0.05f);
	}
}

std::string gbe::editor::StateWindow::GetWindowId()
{
	return "State Control";
}
