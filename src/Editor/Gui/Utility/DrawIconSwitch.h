#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include "Graphics/gbe_graphics.h"

namespace gbe::editor {
    static inline bool DrawIconSwitch(const char* label, bool* v, gfx::TextureData* iconOff, gfx::TextureData* iconOn,
        float trackH = 32.0f,
        float trackW = 80.0f,
        float iconPadding = 6.0f)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

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
        if (!ImGui::ItemAdd(total_bb, id)) return false;

        // --- Interaction ---
        bool hovered, held;
        // ButtonBehavior returns true on the frame the mouse is released after a click
        bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
        if (pressed) *v = !(*v);

        // Animation state
        float t = *v ? 1.0f : 0.0f;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // 1. Draw Background Track
        ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_FrameBg);
        draw_list->AddRectFilled(pos, ImVec2(pos.x + trackW, pos.y + trackH), col_bg, radius);

        // 2. Draw the Elongated Handle
        float handleW = trackW * 0.5f;
        float hPadding = 2.0f; // Gap between handle and track edge

        // Calculate handle movement range
        float minX = pos.x + hPadding;
        float maxX = pos.x + trackW - handleW + hPadding;
        float currentHandleX = minX + (t * (trackW * 0.5f - hPadding * 2));

        ImVec2 handleMin = ImVec2(currentHandleX, pos.y + hPadding);
        ImVec2 handleMax = ImVec2(currentHandleX + handleW - hPadding * 2, pos.y + trackH - hPadding);

        ImU32 col_handle = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        draw_list->AddRectFilled(handleMin, handleMax, col_handle, radius - hPadding);

        // 3. Draw Overlay Icons (Centered in their respective halves)
        float iconSize = trackH - iconPadding * 2;
        float centerY = pos.y + (trackH * 0.5f);
        float slotWidth = trackW * 0.5f;

        // Off Icon Position (Left Half)
        ImVec2 offMin = ImVec2(pos.x + (slotWidth * 0.5f) - (iconSize * 0.5f), centerY - (iconSize * 0.5f));
        draw_list->AddImage((ImTextureID)iconOff->textureHandle.idx, offMin, ImVec2(offMin.x + iconSize, offMin.y + iconSize),
            { 0,0 }, { 1,1 }, *v ? IM_COL32(255, 255, 255, 128) : IM_COL32_WHITE);

        // On Icon Position (Right Half)
        ImVec2 onMin = ImVec2(pos.x + slotWidth + (slotWidth * 0.5f) - (iconSize * 0.5f), centerY - (iconSize * 0.5f));
        draw_list->AddImage((ImTextureID)iconOn->textureHandle.idx, onMin, ImVec2(onMin.x + iconSize, onMin.y + iconSize),
            { 0,0 }, { 1,1 }, *v ? IM_COL32_WHITE : IM_COL32(255, 255, 255, 128));

        // 4. Render Label Text
        if (label_size.x > 0)
            ImGui::RenderText(ImVec2(pos.x + trackW + style.ItemInnerSpacing.x, pos.y + (trackH - label_size.y) * 0.5f), label);

        return pressed;
    }
}