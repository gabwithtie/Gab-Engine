#include "GizmoLayer.h"

#include <imgui.h>
#include <Imguizmo.h>


void gbe::editor::GizmoLayer::DrawSelf()
{
    // Only operate if exactly one object is selected
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
	ImGuizmo::AllowAxisFlip(false);
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImVec2 windowsize = ImGui::GetMainViewport()->Size;
    ImVec2 windowpos = ImGui::GetMainViewport()->Pos;
    ImGuizmo::SetRect(windowpos.x, windowpos.y, windowsize.x, windowsize.y);

    ImGuizmo::OPERATION operation = ImGuizmo::OPERATION();

    if(!selected[0]->GetEditorFlag(Object::STATIC_POS_X))
		operation = operation | ImGuizmo::TRANSLATE_X;
	if (!selected[0]->GetEditorFlag(Object::STATIC_POS_Y))
		operation = operation | ImGuizmo::TRANSLATE_Y;
	if (!selected[0]->GetEditorFlag(Object::STATIC_POS_Z))
		operation = operation | ImGuizmo::TRANSLATE_Z;
	if (!selected[0]->GetEditorFlag(Object::STATIC_ROT_X))
		operation = operation | ImGuizmo::ROTATE_X;
    if (!selected[0]->GetEditorFlag(Object::STATIC_ROT_Y))
		operation = operation | ImGuizmo::ROTATE_Y;
	if (!selected[0]->GetEditorFlag(Object::STATIC_ROT_Z))
		operation = operation | ImGuizmo::ROTATE_Z;
	if (operation == 0)
		return; //All static, no manipulation.

    // Draw the gizmo and handle manipulation
    ImGuizmo::Manipulate(
        view_mat,
        proj_mat,
        operation,
        ImGuizmo::LOCAL,
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
