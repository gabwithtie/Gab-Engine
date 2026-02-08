#include "TexturePainterWindow.h"

#include "../Utility/DrawIconSwitch.h"

#include "Graphics/util/TexturePainter.h"
#include "Editor/Editor.h"

void gbe::editor::TexturePainterWindow::DrawSelf()
{
	auto enabled = TexturePainter::GetEnabled();

	if (DrawIconSwitch("Enable Texture Painter", &enabled))
	{
		TexturePainter::SetEnabled(enabled);
	}
}

void gbe::editor::TexturePainterWindow::Set_is_open(bool newstate)
{
	GuiWindow::Set_is_open(newstate);

	if(!newstate)
		TexturePainter::SetEnabled(false);
}

gbe::editor::TexturePainterWindow::TexturePainterWindow()
{
	Editor::Register_on_mouse_hold([=](Vector2Int pos) {
		if (TexturePainter::GetEnabled())
		{
			TexturePainter::SetTargetTexture(&TextureLoader::GetAssetRuntimeData("Plaster_Albedo"));
			TexturePainter::Draw(pos);
		}
		});
}
