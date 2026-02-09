#include "TexturePainterWindow.h"

#include "../Utility/DrawIconSwitch.h"
#include "../Utility/AssetPickerPopup.h"

#include "Graphics/util/TexturePainter.h"
#include "Editor/Editor.h"

#include "Math/gbe_math.h"

void DrawBrushPreview(int radius, float strength, gbe::Vector4 color) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float previewBoxSize = 120.0f;

    // Draw a dark background box
    drawList->AddRectFilled(pos, { pos.x + previewBoxSize, pos.y + previewBoxSize }, IM_COL32(20, 20, 20, 255));

    ImVec2 center = { pos.x + previewBoxSize / 2.0f, pos.y + previewBoxSize / 2.0f };

    // We draw multiple concentric circles to simulate the smooth falloff
    // This visualizes how 'Strength' and 'Radius' affect the final stamp
    int segments = 20;
    float visualRadius = std::min((float)radius, previewBoxSize / 2.0f - 5.0f);

    for (int i = segments; i > 0; --i) {
        float t = (float)i / (float)segments;
        float currentRadius = visualRadius * t;

        // Simple linear falloff visualization
        float falloff = t; // You can use t*t for a harder brush feel
        float alpha = falloff * strength;

        ImU32 col = ImGui::ColorConvertFloat4ToU32({ color.x, color.y, color.z, alpha });
        drawList->AddCircleFilled(center, currentRadius, col, 32);
    }

    ImGui::Dummy({ previewBoxSize, previewBoxSize });
}

void gbe::editor::TexturePainterWindow::DrawSelf()
{
    auto enabled = TexturePainter::GetEnabled();

    if (DrawIconSwitch("Enable Texture Painter", &enabled))
    {
        TexturePainter::SetEnabled(enabled);
        if (enabled) {
            Editor::HijackPointer(this, [](Vector2Int pos, Editor::PointerState state) {
                if (state == Editor::POINTER_HOLD) {
                    TexturePainter::Draw(pos);
                }
                }, []() {
                    TexturePainter::SetEnabled(false);
                    });
        }
        else {
            Editor::EndPointerHijack();
        }
    }

    ImGui::SeparatorText("Brush Settings");

    // --- 1. Brush Radius Slider ---
    int radius = TexturePainter::GetBrushSize();
    if (ImGui::SliderInt("Radius", &radius, 1, 256)) {
        TexturePainter::SetBrushSize(radius);
    }

    // --- 2. Brush Strength (Alpha/Flow) Slider ---
    float strength = TexturePainter::GetBrushStrength();
    if (ImGui::SliderFloat("Strength", &strength, 0.01f, 1.0f, "%.2f")) {
        TexturePainter::SetBrushStrength(strength);
    }

    // --- 3. Color Picker ---
    // Assuming TexturePainter stores color as a float[4] or Vector4
    Vector4 color = TexturePainter::GetBrushColor();
    if (ImGui::ColorEdit4("Color", &color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) {
		TexturePainter::SetBrushColor(color);
    }

    ImGui::Spacing();

    // --- 4. Debug Brush Preview ---
    ImGui::Text("Brush Preview:");
    DrawBrushPreview(radius, strength, color);

    ImGui::SeparatorText("Target Asset");

    const auto getter = []() { return TexturePainter::GetTargetTexture(); };
    const auto setter = [](std::string id) { TexturePainter::SetTargetTexture(TextureLoader::GetAssetById(id)); };

    TextureAssetPicker("TexturePicker", { 100, 100 }, getter, setter, TextureLoader::GetAllAssetIds());

    if (ImGui::Button("Re-save")) {
        TexturePainter::ApplyAndSave();
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
}
