#include "ViewportWindow.h"
#include "Graphics/gbe_graphics.h"
#include "Engine/gbe_engine.h"

std::vector<gbe::editor::IconCommand> gbe::editor::ViewportWindow::iconQueue;

gbe::editor::ViewportWindow::ViewportWindow(std::vector<gbe::Object*>& _selected) :
    gizmoLayer(_selected)
{
    selected_data = &gfx::TextureLoader::GetDataMap().at("mainpass");
}

void gbe::editor::ViewportWindow::DrawSelf()
{
    // 1. Setup and capture viewport state
    viewportSize = ImGui::GetContentRegionAvail();
    viewportScreenPos = ImGui::GetCursorScreenPos();
    Vector2Int cur_resolution = { (int)viewportSize.x, (int)viewportSize.y };

    if (old_resolution.x != cur_resolution.x || old_resolution.y != cur_resolution.y) {
        old_resolution = cur_resolution;
        RenderPipeline::SetViewportResolution(cur_resolution, { (int)viewportScreenPos.x, (int)viewportScreenPos.y });
    }

    // 2. Render Background Viewport
    ImGui::Image((ImTextureID)this->selected_data->textureHandle.idx, viewportSize, { 0, 0 }, { 1, 1 });

    // 3. Execute Cached Icon Commands
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float iconSize = 100.0f;

    for (const auto& cmd : iconQueue) {
        // Project 3D -> 2D
        Vector4 worldPos = { cmd.position.x, cmd.position.y, cmd.position.z, 1.0f };
        Vector4 clipSpacePos = cmd.proj * (cmd.view * worldPos);

        if (clipSpacePos.w <= 0.0f) continue; // Behind camera

        Vector2 ndc = { clipSpacePos.x / clipSpacePos.w, clipSpacePos.y / clipSpacePos.w };

        // Map to Screen Space using captured viewportScreenPos
        float screenX = viewportScreenPos.x + (ndc.x + 1.0f) * 0.5f * viewportSize.x;
        float screenY = viewportScreenPos.y + (1.0f - ndc.y) * 0.5f * viewportSize.y;

        ImTextureID iconID = (ImTextureID)cmd.tex.textureHandle.idx;
        ImVec2 p_min = { screenX - iconSize * 0.5f, screenY - iconSize * 0.5f };
        ImVec2 p_max = { screenX + iconSize * 0.5f, screenY + iconSize * 0.5f };

        drawList->AddImage(iconID, p_min, p_max, { 0, 0 }, { 1, 1 }, IM_COL32_WHITE);
    }

    // 4. Clear the queue for the next frame
    iconQueue.clear();

    // 5. Draw Gizmos
    this->gizmoLayer.Draw();

    if (this->gizmoLayer.Get_pointer_here())
        this->pointer_here = false;
}

void gbe::editor::ViewportWindow::RenderIcon(gfx::TextureData& tex, const Vector3& position, const Matrix4& camera_view, const Matrix4& camera_proj)
{
    // Simply push to the queue to be processed during DrawSelf
    iconQueue.push_back({ tex, position, camera_view, camera_proj });
}