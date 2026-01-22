#include "ConsoleWindow.h"

#include "Engine/gbe_engine.h"

void gbe::editor::ConsoleWindow::DrawSelf()
{
	for (const auto log : this->logs)
	{
		ImGui::Text(log.c_str());
	}
}

std::string gbe::editor::ConsoleWindow::GetWindowId()
{
	return "Console";
}

void gbe::editor::ConsoleWindow::receive_log(std::string log)
{
	this->logs.push_back(log);
}

gbe::editor::ConsoleWindow::ConsoleWindow()
{
	gbe::Console::Subscribe([this](std::string log) {
		this->receive_log(log);
		});
}
