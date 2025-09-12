#include "GizmoLayer.h"

#include <imgui.h>
#include <Imguizmo.h>


void gbe::editor::GizmoLayer::DrawSelf()
{
    if (selected.size() != 1)
        return;

    // Get references
    auto camera = Engine::GetActiveCamera();
    auto cam_view = camera->GetViewMat();
    auto cam_proj = camera->GetProjectionMat();

    // Convert GLM matrices to C-style arrays for ImGuizmo
    float view_mat[16];
    float proj_mat[16];
    float model_mat[16];

    // Copy GLM data to C-style arrays
    memcpy(view_mat, cam_view.Get_Ptr(), sizeof(float) * 16);
    memcpy(proj_mat, cam_proj.Get_Ptr(), sizeof(float) * 16);
    memcpy(model_mat, selected[0]->World().GetMatrix().Get_Ptr(), sizeof(float) * 16);

    // IMGUI SETUP
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImVec2 windowsize = ImGui::GetMainViewport()->Size;
    ImVec2 windowpos = ImGui::GetMainViewport()->Pos;
    ImGuizmo::SetRect(windowpos.x, windowpos.y, windowsize.x, windowsize.y);

    // Draw the gizmo and handle manipulation
    ImGuizmo::Manipulate(
        view_mat,
        proj_mat,
        ImGuizmo::TRANSLATE | ImGuizmo::ROTATE,
        ImGuizmo::WORLD,
        model_mat
    );

    // Check if the gizmo was actively used
    if (ImGuizmo::IsUsing()) {
        // Copy the updated data back to your GLM matrix
        selected[0]->Local().SetMatrix(Matrix4(model_mat));
        selected[0]->PushState(Object::TRANSFORMED_USER);
    }
}

bool gbe::editor::GizmoLayer::ext_Begin()
{
    return true;
}

void gbe::editor::GizmoLayer::ext_End()
{
}
