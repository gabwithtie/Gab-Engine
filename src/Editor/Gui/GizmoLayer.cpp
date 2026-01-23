#include "GizmoLayer.h"

#include <imgui.h>
#include <Imguizmo.h>

#include "Engine/gbe_engine.h"

void gbe::editor::GizmoLayer::DrawSelf()
{
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
    Matrix4 totalmodel = Matrix4(1);

    if (selected.size() == 1)
        totalmodel = selected[0]->World().GetMatrix();
    else {
        Vector3 total_pos(0);

        for (const auto& selectedobj : selected)
        {
            total_pos += selectedobj->World().position.Get();
        }

        total_pos /= (float)selected.size();
        totalmodel[3] = Vector4(total_pos, 1);
    }
    memcpy(model_mat, totalmodel.Get_Ptr(), sizeof(float) * 16);

    // IMGUI SETUP
	ImGuizmo::AllowAxisFlip(false);
    ImGuizmo::SetDrawlist();
    ImVec2 windowsize = ImGui::GetWindowSize();
    ImVec2 windowpos = ImGui::GetWindowPos();
    ImGuizmo::SetRect(windowpos.x, windowpos.y, windowsize.x, windowsize.y);

    ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::UNIVERSAL;

    for (const auto& selectedobj : selected)
    {
        if (selectedobj->GetEditorFlag(Object::STATIC_POS_X))
            operation = static_cast<ImGuizmo::OPERATION>(operation & ~ImGuizmo::TRANSLATE_X);
        if (selectedobj->GetEditorFlag(Object::STATIC_POS_Y))
            operation = static_cast<ImGuizmo::OPERATION>(operation & ~ImGuizmo::TRANSLATE_Y);
        if (selectedobj->GetEditorFlag(Object::STATIC_POS_Z))
            operation = static_cast<ImGuizmo::OPERATION>(operation & ~ImGuizmo::TRANSLATE_Z);
        if (selectedobj->GetEditorFlag(Object::STATIC_ROT_X))
            operation = static_cast<ImGuizmo::OPERATION>(operation & ~ImGuizmo::ROTATE_X & ~ImGuizmo::ROTATE_SCREEN);
        if (selectedobj->GetEditorFlag(Object::STATIC_ROT_Y))
            operation = static_cast<ImGuizmo::OPERATION>(operation & ~ImGuizmo::ROTATE_Y & ~ImGuizmo::ROTATE_SCREEN);
        if (selectedobj->GetEditorFlag(Object::STATIC_ROT_Z))
            operation = static_cast<ImGuizmo::OPERATION>(operation & ~ImGuizmo::ROTATE_Z & ~ImGuizmo::ROTATE_SCREEN);
    }

    if (selected.size() > 1 || operation != ImGuizmo::OPERATION::UNIVERSAL) {
        operation = static_cast<ImGuizmo::OPERATION>(operation & ~ImGuizmo::SCALEU);
        operation = static_cast<ImGuizmo::OPERATION>(operation & ~ImGuizmo::SCALE);
    }

	if (operation == 0)
		return; //All static, no manipulation.

    if (selected.size() == 1) {
        // Draw the gizmo and handle manipulation
        ImGuizmo::Manipulate(
            view_mat,
            proj_mat,
            operation,
            ImGuizmo::LOCAL,
            model_mat
        );

        pointer_here = ImGuizmo::IsOver();

        // Check if the gizmo was actively used
        if (ImGuizmo::IsUsing()) {
            // Copy the updated data back to your GLM matrix
            selected[0]->World().SetMatrix(gbe::Matrix4(model_mat));
            selected[0]->PushState(Object::TRANSFORMED_USER);
        }
    }
    else {
        // 1. Create a copy of the center matrix before manipulation
        Matrix4 old_center_mat = totalmodel;

        // 2. Manipulate the group center matrix
        ImGuizmo::Manipulate(
            view_mat,
            proj_mat,
            operation,
            ImGuizmo::WORLD, // Usually better for multi-select
            model_mat
        );

        pointer_here = ImGuizmo::IsOver();

        if (ImGuizmo::IsUsing()) {
            // 3. Calculate the Delta Matrix: NewCenter * Inverse(OldCenter)
            Matrix4 new_center_mat = gbe::Matrix4(model_mat);
            Matrix4 delta_mat = new_center_mat * old_center_mat.Inverted();

            // 4. Apply the same delta to every object
            for (auto& obj : selected) {
                Matrix4 current_obj_mat = obj->World().GetMatrix();

                // New world matrix = Delta * CurrentWorld
                // (The order depends on if your Matrix class is Row or Column major)
                Matrix4 updated_mat = delta_mat * current_obj_mat;

                obj->World().SetMatrix(updated_mat);
                obj->PushState(Object::TRANSFORMED_USER);
            }
        }
    }
}

bool gbe::editor::GizmoLayer::ext_Begin()
{
    return true;
}

void gbe::editor::GizmoLayer::ext_End()
{
}
