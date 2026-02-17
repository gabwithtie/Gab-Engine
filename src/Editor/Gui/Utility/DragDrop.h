#pragma once

#include <imgui.h>

#include "Asset/AssetTypes/Types.h"

namespace gbe::editor {
    struct DragData {
        asset::AssetType datatype;
        std::string id;
    };

    inline void DraggableTarget( asset::AssetType datatype, std::function<void(const DragData&)> onDrop, std::function<void()> uiFunc) {
        uiFunc(); // Render the element that will act as the target

        if (ImGui::BeginDragDropTarget()) {
            if(auto payload = ImGui::AcceptDragDropPayload(asset::GetTypeString(datatype)))
            {
                IM_ASSERT(payload->DataSize == sizeof(DragData));
                DragData& droppedData = *(DragData*)payload->Data;
                onDrop(droppedData); // Execute the logic for the dropped data
            }
            ImGui::EndDragDropTarget();
        }
    }

    inline void DraggableSource(asset::AssetType datatype, const DragData& data, std::function<void()> uiFunc) {
        uiFunc(); // Render the actual element

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            // Set the payload (carrying the data, like a pointer or string ID)
            ImGui::SetDragDropPayload(asset::GetTypeString(datatype), &data, sizeof(DragData));

            // Visual preview that follows the mouse
            ImGui::Text("Dragging %s...", asset::GetTypeString(datatype));
            ImGui::EndDragDropSource();
        }
    }
}

