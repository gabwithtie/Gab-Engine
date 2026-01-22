#include "CreditsWindow.h"

gbe::editor::CreditsWindow::CreditsWindow():
	logo_tex_data(TextureLoader::GetAssetRuntimeData(this->logo_tex->Get_assetId()))
{
	this->logo_tex = new gbe::asset::Texture("DefaultAssets/Tex/UI/logo.img.gbe");
}

void gbe::editor::CreditsWindow::DrawSelf() {
	ImGui::Image((ImTextureID)TextureLoader::GetGuiHandle(&logo_tex_data).idx, {128, 128}, {0, 1}, {1, 0});
	ImGui::Text("About");
	ImGui::Text("GabEngine v0.1");
	ImGui::Text("Developed by Gabriel Rayo");
	ImGui::Text("Acknowledgements:");
	ImGui::Text("De La Salle University");
	ImGui::Text("GDGRAP");
	ImGui::Text("GDPHYSX");
	ImGui::Text("GDENG03");
}

std::string gbe::editor::CreditsWindow::GetWindowId()
{
	return "Credits";
}