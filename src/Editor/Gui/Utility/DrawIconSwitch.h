#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include "Graphics/gbe_graphics.h"

namespace gbe::editor {
    static inline void DrawIconSwitch(const char* label, bool* v, gfx::TextureData* iconOff, gfx::TextureData* iconOn,
        float trackH = 32.0f,
        float trackW = 80.0f,
        float iconPadding = 6.0f)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        // --- Sizing ---
        ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
        ImVec2 pos = window->DC.CursorPos;
        float radius = trackH * 0.5f;

        // Total bounding box (track + text)
        ImRect total_bb(pos, ImVec2(pos.x + trackW + (label_size.x > 0 ? style.ItemInnerSpacing.x + label_size.x : 0), pos.y + trackH));
        ImGui::ItemSize(total_bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(total_bb, id)) return;

        // --- Interaction ---
        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
        if (pressed) *v = !(*v);

        // Animation state (0.0 for off, 1.0 for on)
        float t = *v ? 1.0f : 0.0f;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // 1. Draw Background Track
        ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
        draw_list->AddRectFilled(pos, ImVec2(pos.x + trackW, pos.y + trackH), col_bg, radius);

        // 2. Draw the Elongated Handle (The "Slider")
        // The handle is half the width of the track
        float handleW = trackW * 0.5f;
        float handlePadding = 2.0f;
        ImVec2 handleMin = ImVec2(pos.x + handlePadding + (t * (trackW * 0.5f - handlePadding * 2)), pos.y + handlePadding);
        ImVec2 handleMax = ImVec2(handleMin.x + handleW - handlePadding, pos.y + trackH - handlePadding);

        ImU32 col_handle = ImGui::GetColorU32(ImGuiCol_ButtonActive);
        draw_list->AddRectFilled(handleMin, handleMax, col_handle, radius - handlePadding);

        // 3. Draw Overlay Icons
        // These sit ON TOP of the handle and track
        float iconSize = trackH - iconPadding * 2;
        float centerX_Off = pos.x + (trackW * 0.25f); // Center of left half
        float centerX_On = pos.x + (trackW * 0.75f);  // Center of right half
        float centerY = pos.y + (trackH * 0.5f);

        // Off Icon (Left)
        ImVec2 offMin = ImVec2(centerX_Off - iconSize * 0.5f, centerY - iconSize * 0.5f);
        draw_list->AddImage((ImTextureID)iconOff->textureHandle.idx, offMin, ImVec2(offMin.x + iconSize, offMin.y + iconSize), { 0,0 }, { 1,1 }, *v ? IM_COL32(255, 255, 255, 150) : IM_COL32_WHITE);

        // On Icon (Right)
        ImVec2 onMin = ImVec2(centerX_On - iconSize * 0.5f, centerY - iconSize * 0.5f);
        draw_list->AddImage((ImTextureID)iconOn->textureHandle.idx, onMin, ImVec2(onMin.x + iconSize, onMin.y + iconSize), { 0,0 }, { 1,1 }, *v ? IM_COL32_WHITE : IM_COL32(255, 255, 255, 150));

        // 4. Render Label Text
        if (label_size.x > 0)
            ImGui::RenderText(ImVec2(pos.x + trackW + style.ItemInnerSpacing.x, pos.y + (trackH - label_size.y) * 0.5f), label);
    }
}